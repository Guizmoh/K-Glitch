package com.somnia.app.service

import android.Manifest
import android.annotation.SuppressLint
import android.app.Notification
import android.app.PendingIntent
import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import android.content.pm.ServiceInfo
import android.hardware.Sensor
import android.hardware.SensorEvent
import android.hardware.SensorEventListener
import android.hardware.SensorManager
import android.media.AudioFormat
import android.media.AudioRecord
import android.media.MediaRecorder
import android.os.Build
import android.os.PowerManager
import androidx.core.app.NotificationCompat
import androidx.core.app.ServiceCompat
import androidx.core.content.ContextCompat
import androidx.lifecycle.LifecycleService
import androidx.lifecycle.lifecycleScope
import com.somnia.app.R
import com.somnia.app.SomniaApp
import com.somnia.app.alarm.AlarmScheduler
import com.somnia.app.analysis.MinuteInput
import com.somnia.app.analysis.SleepAnalyzer
import com.somnia.app.analysis.Stage
import com.somnia.app.audio.AudioMath
import com.somnia.app.audio.CapturedEvent
import com.somnia.app.audio.EventClassifier
import com.somnia.app.audio.HeuristicClassifier
import com.somnia.app.audio.SoundClassifier
import com.somnia.app.audio.SoundEventDetector
import com.somnia.app.audio.SoundType
import com.somnia.app.audio.VoiceEnhancer
import com.somnia.app.audio.WavFile
import com.somnia.app.audio.YamnetClassifier
import com.somnia.app.data.MinuteStat
import com.somnia.app.data.Settings
import com.somnia.app.data.SettingsRepository
import com.somnia.app.data.SleepDao
import com.somnia.app.data.SomniaDatabase
import com.somnia.app.data.SoundEventEntity
import com.somnia.app.ui.MainActivity
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.NonCancellable
import kotlinx.coroutines.channels.Channel
import kotlinx.coroutines.isActive
import kotlinx.coroutines.launch
import kotlinx.coroutines.runBlocking
import kotlinx.coroutines.withContext
import java.io.File
import java.util.Calendar
import kotlin.math.sqrt

/**
 * Service de premier plan qui écoute la nuit :
 *  - lit le micro en continu (16 kHz) et repère les sons qui émergent du bruit de fond ;
 *  - classe chaque son (paroles, cris, ronflement, rire, pleurs, toux...) ;
 *  - n'enregistre sur le téléphone que ces passages (pas toute la nuit) ;
 *  - mesure les mouvements via l'accéléromètre (téléphone posé sur le matelas) ;
 *  - déclenche le réveil intelligent en phase de sommeil léger.
 */
class SleepTrackingService : LifecycleService(), SensorEventListener {

    private lateinit var dao: SleepDao
    private lateinit var settings: Settings
    private var sessionId = 0L
    private var startTime = 0L
    private var audioJob: Job? = null
    private var processJob: Job? = null
    private var minuteJob: Job? = null
    private val events = Channel<CapturedEvent>(Channel.UNLIMITED)
    private var classifier: SoundClassifier? = null
    private var wakeLock: PowerManager.WakeLock? = null
    private var sensorManager: SensorManager? = null

    // Accumulateurs de la minute en cours (thread audio / capteur -> minuteJob)
    private val lock = Any()
    private var minuteDbSum = 0.0
    private var minuteDbCount = 0
    private var minuteDbMax = -96f
    private var minuteMovement = 0f
    private var minuteEvents = 0
    private val gravity = FloatArray(3)
    private var gravityInit = false
    private val recentMovement = ArrayDeque<Float>()
    private var floorSum = 0.0
    private var floorCount = 0
    private var alarmTarget: Long? = null
    private var alarmFired = false
    @Volatile private var stopping = false

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        super.onStartCommand(intent, flags, startId)
        when (intent?.action) {
            ACTION_STOP -> stopTracking()
            else -> if (sessionId == 0L) startTracking()
        }
        return START_STICKY
    }

    @SuppressLint("WakelockTimeout")
    private fun startTracking() {
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.RECORD_AUDIO) != PackageManager.PERMISSION_GRANTED) {
            stopSelf(); return
        }
        dao = SomniaDatabase.get(this).dao()
        settings = SettingsRepository.get(this).settings.value
        startTime = System.currentTimeMillis()
        ServiceCompat.startForeground(
            this, NOTIF_ID, buildNotification("Écoute de la nuit en cours…"),
            if (Build.VERSION.SDK_INT >= 30) ServiceInfo.FOREGROUND_SERVICE_TYPE_MICROPHONE else 0,
        )
        wakeLock = (getSystemService(Context.POWER_SERVICE) as PowerManager)
            .newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, "somnia:tracking").apply { acquire() }

        lifecycleScope.launch {
            val c = withContext(Dispatchers.Default) { YamnetClassifier.create(this@SleepTrackingService) ?: HeuristicClassifier() }
            classifier = c
            // Une nuit précédente interrompue (téléphone éteint...) est clôturée.
            dao.openSession()?.let { finalizeSession(it.id) }
            sessionId = dao.insertSession(
                com.somnia.app.data.SleepSession(
                    startTime = startTime, micSensitivity = settings.micSensitivity, classifierName = c.name,
                ),
            )
            if (settings.smartAlarmEnabled) {
                alarmTarget = nextOccurrence(settings.alarmHour, settings.alarmMinute).also {
                    AlarmScheduler.scheduleAlarm(this@SleepTrackingService, it)
                }
            }
            TrackingState.live.value = LiveState(
                sessionId = sessionId, startTime = startTime,
                detectionStartsAt = startTime + settings.startDelayMin * 60_000L,
                classifierName = c.name, alarmTarget = alarmTarget,
            )
            if (settings.trackMovement) startMovement()
            startProcessing(c)
            startAudio()
            startMinuteLoop()
        }
    }

    private fun nextOccurrence(h: Int, m: Int): Long {
        val cal = Calendar.getInstance().apply {
            set(Calendar.HOUR_OF_DAY, h); set(Calendar.MINUTE, m); set(Calendar.SECOND, 0); set(Calendar.MILLISECOND, 0)
        }
        if (cal.timeInMillis <= System.currentTimeMillis() + 60_000) cal.add(Calendar.DAY_OF_YEAR, 1)
        return cal.timeInMillis
    }

    @SuppressLint("MissingPermission")
    private fun startAudio() {
        audioJob = lifecycleScope.launch(Dispatchers.IO) {
            val sr = AudioMath.SAMPLE_RATE
            val minBuf = AudioRecord.getMinBufferSize(sr, AudioFormat.CHANNEL_IN_MONO, AudioFormat.ENCODING_PCM_16BIT)
            val bufSize = maxOf(minBuf, AudioMath.FRAME * 8)
            // VOICE_RECOGNITION : flux « brut » sans réduction de bruit automatique,
            // indispensable pour entendre ronflements et chuchotements.
            val record = runCatching {
                AudioRecord(MediaRecorder.AudioSource.VOICE_RECOGNITION, sr, AudioFormat.CHANNEL_IN_MONO, AudioFormat.ENCODING_PCM_16BIT, bufSize)
            }.getOrNull()?.takeIf { it.state == AudioRecord.STATE_INITIALIZED }
                ?: AudioRecord(MediaRecorder.AudioSource.MIC, sr, AudioFormat.CHANNEL_IN_MONO, AudioFormat.ENCODING_PCM_16BIT, bufSize)
            val detector = SoundEventDetector().apply { setSensitivity(settings.micSensitivity) }
            val delayUntil = startTime + settings.startDelayMin * 60_000L
            record.startRecording()
            var uiTick = 0
            try {
                while (isActive && !stopping) {
                    val frame = ShortArray(AudioMath.FRAME)
                    var read = 0
                    while (read < frame.size && isActive) {
                        val r = record.read(frame, read, frame.size - read)
                        if (r < 0) break
                        read += r
                    }
                    if (read < frame.size) continue
                    val ev = detector.process(frame)
                    synchronized(lock) {
                        minuteDbSum += detector.lastDb; minuteDbCount++
                        if (detector.lastDb > minuteDbMax) minuteDbMax = detector.lastDb
                    }
                    if (ev != null && System.currentTimeMillis() >= delayUntil) events.trySend(ev)
                    if (++uiTick >= 5) { // rafraîchit l'écran 2 fois par seconde
                        uiTick = 0
                        floorSum += detector.noiseFloorDb; floorCount++
                        TrackingState.live.value = TrackingState.live.value?.copy(
                            levelDb = detector.lastDb, noiseFloorDb = detector.noiseFloorDb,
                            thresholdDb = detector.threshold, capturing = detector.isRecording,
                        )
                    }
                }
                detector.flush()?.let { events.trySend(it) }
            } finally {
                record.stop(); record.release()
            }
        }
    }

    private fun startProcessing(c: SoundClassifier) {
        val ec = EventClassifier(c, settings.thresholds())
        val dir = File(filesDir, "recordings/$sessionId").apply { mkdirs() }
        processJob = lifecycleScope.launch(Dispatchers.Default) {
            for (ev in events) {
                val cls = ec.classify(ev.pcm)
                val ts = startTime + ev.startOffsetMs
                val keepAudio = cls.type in settings.recordTypes
                if (cls.type == SoundType.NOISE && SoundType.NOISE !in settings.recordTypes) {
                    synchronized(lock) { minuteEvents++ }
                    continue // bruit non identifié : on ne garde rien
                }
                var audio: String? = null
                var enhanced: String? = null
                if (keepAudio) {
                    val f = File(dir, "${ts}_${cls.type.name.lowercase()}.wav")
                    WavFile.write(f, ev.pcm)
                    audio = f.absolutePath
                    val voiceLike = cls.type in setOf(SoundType.SPEECH, SoundType.NIGHT_TERROR, SoundType.LAUGHTER, SoundType.CRYING)
                    if (settings.enhanceVoice && voiceLike) {
                        val e = File(dir, "${ts}_${cls.type.name.lowercase()}_voix.wav")
                        WavFile.write(e, VoiceEnhancer.enhance(ev.pcm, ev.noiseSample))
                        enhanced = e.absolutePath
                    }
                }
                dao.insertEvent(
                    SoundEventEntity(
                        sessionId = sessionId, timestamp = ts, durationMs = ev.durationMs,
                        type = cls.type.name, confidence = cls.confidence, peakDb = ev.peakDb,
                        noiseFloorDb = ev.noiseFloorDb,
                        scores = cls.scores.entries.sortedByDescending { it.value }
                            .joinToString(";") { "${it.key.name}=${"%.2f".format(java.util.Locale.US, it.value)}" },
                        audioPath = audio, enhancedPath = enhanced,
                    ),
                )
                synchronized(lock) { minuteEvents++ }
                TrackingState.live.value = TrackingState.live.value?.let { s ->
                    s.copy(
                        eventCount = s.eventCount + 1, lastEvent = cls.type, lastEventTime = ts,
                        counts = s.counts + (cls.type to ((s.counts[cls.type] ?: 0) + 1)),
                    )
                }
                updateNotification()
            }
        }
    }

    private fun startMinuteLoop() {
        minuteJob = lifecycleScope.launch {
            var minute = 0
            while (isActive) {
                val next = startTime + (minute + 1) * 60_000L
                kotlinx.coroutines.delay((next - System.currentTimeMillis()).coerceAtLeast(1000))
                writeMinute(minute)
                minute++
                checkSmartAlarm()
            }
        }
    }

    private suspend fun writeMinute(minute: Int) {
        val stat = synchronized(lock) {
            val s = MinuteStat(
                sessionId = sessionId, minute = minute, movement = minuteMovement,
                avgDb = if (minuteDbCount == 0) -90f else (minuteDbSum / minuteDbCount).toFloat(),
                maxDb = minuteDbMax, events = minuteEvents,
            )
            minuteDbSum = 0.0; minuteDbCount = 0; minuteDbMax = -96f; minuteMovement = 0f; minuteEvents = 0
            s
        }
        recentMovement.addLast(stat.movement)
        while (recentMovement.size > 60) recentMovement.removeFirst()
        TrackingState.live.value = TrackingState.live.value?.copy(movementLastMin = stat.movement)
        dao.upsertMinute(stat)
    }

    /** Dans la fenêtre du réveil, sonne dès qu'un mouvement signale un sommeil léger. */
    private fun checkSmartAlarm() {
        val target = alarmTarget ?: return
        if (alarmFired) return
        val now = System.currentTimeMillis()
        if (now < target - settings.alarmWindowMin * 60_000L || now >= target) return
        if (!settings.trackMovement || recentMovement.size < 10) return
        val sorted = recentMovement.sorted()
        val median = sorted[sorted.size / 2]
        val last = recentMovement.last()
        if (last > maxOf(median * 2.5f, 1.5f)) {
            alarmFired = true
            AlarmScheduler.cancelAlarm(this)
            AlarmScheduler.ringNow(this)
        }
    }

    private fun startMovement() {
        val sm = getSystemService(Context.SENSOR_SERVICE) as SensorManager
        val acc = sm.getDefaultSensor(Sensor.TYPE_ACCELEROMETER) ?: return
        sensorManager = sm
        sm.registerListener(this, acc, SensorManager.SENSOR_DELAY_NORMAL)
    }

    override fun onSensorChanged(e: SensorEvent) {
        // Filtre passe-haut : on retire la gravité pour ne garder que les secousses.
        val a = 0.9f
        if (!gravityInit) { e.values.copyInto(gravity, 0, 0, 3); gravityInit = true; return }
        for (i in 0..2) gravity[i] = a * gravity[i] + (1 - a) * e.values[i]
        val x = e.values[0] - gravity[0]; val y = e.values[1] - gravity[1]; val z = e.values[2] - gravity[2]
        val mag = sqrt(x * x + y * y + z * z)
        if (mag > 0.05f) synchronized(lock) { minuteMovement += (mag - 0.05f) }
    }

    override fun onAccuracyChanged(sensor: Sensor?, accuracy: Int) {}

    private fun stopTracking() {
        if (stopping) return
        stopping = true
        sensorManager?.unregisterListener(this)
        lifecycleScope.launch {
            withContext(NonCancellable) {
                audioJob?.join()
                events.close()
                processJob?.join()
                minuteJob?.cancel()
                if (sessionId != 0L) {
                    writeMinute(((System.currentTimeMillis() - startTime) / 60_000L).toInt())
                    finalizeSession(sessionId)
                }
                AlarmScheduler.cancelAlarm(this@SleepTrackingService)
                cleanupOldRecordings()
            }
            TrackingState.live.value = null
            classifier?.close()
            wakeLock?.let { if (it.isHeld) it.release() }
            ServiceCompat.stopForeground(this@SleepTrackingService, ServiceCompat.STOP_FOREGROUND_REMOVE)
            stopSelf()
        }
    }

    private suspend fun finalizeSession(id: Long) {
        val session = dao.session(id) ?: return
        val minutes = dao.minutes(id)
        val evs = dao.eventsList(id)
        val end = session.endTime ?: maxOf(
            System.currentTimeMillis().takeIf { id == sessionId } ?: 0L,
            session.startTime + minutes.size * 60_000L,
        )
        fun minuteOf(t: Long) = ((t - session.startTime) / 60_000L).toInt()
        val snoring = evs.filter { it.type == SoundType.SNORING.name }.flatMap { e ->
            (minuteOf(e.timestamp)..minuteOf(e.timestamp + e.durationMs)).toList()
        }.toSet()
        val disturbing = evs.filter {
            it.type in setOf(SoundType.SPEECH, SoundType.NIGHT_TERROR, SoundType.CRYING, SoundType.LAUGHTER, SoundType.COUGH).map { t -> t.name }
        }.groupingBy { minuteOf(it.timestamp) }.eachCount()
        val input = minutes.map {
            MinuteInput(it.minute, it.movement, disturbing[it.minute] ?: 0, it.minute in snoring)
        }
        val a = SleepAnalyzer.analyze(input, settings.sleepGoalMin)
        dao.upsertMinutes(minutes.mapIndexed { i, m ->
            m.copy(stage = when (a.stages.getOrNull(i)) { Stage.DEEP -> 0; Stage.AWAKE -> 2; else -> 1 })
        })
        dao.updateSession(
            session.copy(
                endTime = end,
                sleepOnset = a.onsetMinute?.let { session.startTime + it * 60_000L },
                wakeTime = a.wakeMinute?.let { session.startTime + it * 60_000L },
                avgNoiseFloorDb = if (floorCount > 0 && id == sessionId) (floorSum / floorCount).toFloat() else session.avgNoiseFloorDb,
                score = if (minutes.size >= 30) a.score else null,
                deepMinutes = a.deepMinutes, lightMinutes = a.lightMinutes, awakeMinutes = a.awakeMinutes,
                snoringMinutes = a.snoringMinutes, awakenings = a.awakenings,
            ),
        )
    }

    private suspend fun cleanupOldRecordings() {
        val days = settings.retentionDays
        if (days <= 0) return
        val before = System.currentTimeMillis() - days * 86_400_000L
        for (e in dao.oldRecordings(before)) {
            e.audioPath?.let { File(it).delete() }
            e.enhancedPath?.let { File(it).delete() }
            dao.updateEvent(e.copy(audioPath = null, enhancedPath = null))
        }
    }

    private fun buildNotification(text: String): Notification {
        val open = PendingIntent.getActivity(
            this, 0, Intent(this, MainActivity::class.java), PendingIntent.FLAG_IMMUTABLE,
        )
        val stop = PendingIntent.getService(
            this, 1, Intent(this, SleepTrackingService::class.java).setAction(ACTION_STOP), PendingIntent.FLAG_IMMUTABLE,
        )
        return NotificationCompat.Builder(this, SomniaApp.CH_TRACKING)
            .setSmallIcon(R.drawable.ic_moon)
            .setContentTitle("Somnia écoute votre nuit")
            .setContentText(text)
            .setOngoing(true)
            .setContentIntent(open)
            .addAction(0, "Je me réveille", stop)
            .build()
    }

    private fun updateNotification() {
        val s = TrackingState.live.value ?: return
        val text = if (s.counts.isEmpty()) "Aucun son détecté pour l'instant"
        else s.counts.entries.joinToString(" · ") { "${it.key.emoji} ${it.value}" }
        androidx.core.app.NotificationManagerCompat.from(this).let {
            if (ContextCompat.checkSelfPermission(this, Manifest.permission.POST_NOTIFICATIONS) == PackageManager.PERMISSION_GRANTED ||
                Build.VERSION.SDK_INT < 33
            ) it.notify(NOTIF_ID, buildNotification(text))
        }
    }

    override fun onDestroy() {
        sensorManager?.unregisterListener(this)
        wakeLock?.let { if (it.isHeld) it.release() }
        if (!stopping && sessionId != 0L) {
            // Arrêt brutal : on sauvegarde au mieux.
            runBlocking { runCatching { finalizeSession(sessionId) } }
            TrackingState.live.value = null
        }
        super.onDestroy()
    }

    companion object {
        const val ACTION_STOP = "com.somnia.app.STOP"
        private const val NOTIF_ID = 42

        fun start(context: Context) =
            ContextCompat.startForegroundService(context, Intent(context, SleepTrackingService::class.java))

        fun stop(context: Context) =
            context.startService(Intent(context, SleepTrackingService::class.java).setAction(ACTION_STOP))
    }
}
