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
    private val gSum = DoubleArray(3)
    private var gCount = 0
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
            this, NOTIF_ID, buildNotification("Surveillance de la nuit en cours…"),
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
            com.somnia.app.widget.DodoWidget.refresh(this@SleepTrackingService)
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
            val rec = com.somnia.app.audio.MicChoice.open(this@SleepTrackingService, settings.micSource, hiRes = true)
            val record = rec.record
            val factor = (rec.rate / AudioMath.SAMPLE_RATE).coerceAtLeast(1)
            val raw = ShortArray(AudioMath.FRAME * factor * rec.channels)
            val detector = SoundEventDetector().apply { setSensitivity(settings.micSensitivity) }
            val tv = com.somnia.app.audio.TvDetector()
            // Pauses respiratoires : analyse continue + mémoire des 50 dernières secondes de son
            val apnea = com.somnia.app.audio.ApneaDetector()
            val apneaTv = com.somnia.app.audio.TvDetector() // la télé met le suivi en pause, même si le mode télé est éteint
            val ring = ArrayDeque<ShortArray>()
            var fi = 0L
            val delayUntil = startTime + settings.startDelayMin * 60_000L
            record.startRecording()
            TrackingState.live.value = TrackingState.live.value?.copy(micInfo = rec.describe())
            var uiTick = 0
            try {
                while (isActive && !stopping) {
                    var read = 0
                    while (read < raw.size && isActive) {
                        val r = record.read(raw, read, raw.size - read)
                        if (r < 0) break
                        read += r
                    }
                    if (read < raw.size) continue
                    // « Tous les micros » : on mélange les canaux en un seul
                    val mono = if (rec.channels == 1) raw.copyOf() else ShortArray(AudioMath.FRAME * factor) { i ->
                        var sum = 0
                        for (c in 0 until rec.channels) sum += raw[i * rec.channels + c]
                        (sum / rec.channels).toShort()
                    }
                    // Analyse en 16 kHz ; le son 48 kHz est gardé pour l'enregistrement
                    val frame = com.somnia.app.audio.Resample.down(mono, factor)
                    val hi = if (factor > 1) mono else null
                    if (settings.tvMode) {
                        tv.process(AudioMath.rmsDb(frame), detector.noiseFloorDb)
                        detector.mediaLevelDb = if (tv.isOn) tv.tvLevelDb else -96f
                        if (tv.isOn != (TrackingState.live.value?.tvOn ?: false)) {
                            TrackingState.live.value = TrackingState.live.value?.copy(tvOn = tv.isOn)
                        }
                    }
                    // Sons pour s'endormir en cours : la détection est en pause (seuil hors d'atteinte)
                    val soundsOn = com.somnia.app.audio.SleepSounds.activeOrRecent()
                    if (soundsOn) detector.mediaLevelDb = 0f
                    else if (!settings.tvMode || !tv.isOn) detector.mediaLevelDb = -96f
                    val ev = detector.process(frame, hi, rec.rate)
                    if (settings.apneaMode) {
                        fi++
                        ring.addLast(hi ?: frame); while (ring.size > 500) ring.removeFirst()
                        apneaTv.process(detector.lastDb, detector.noiseFloorDb)
                        val ep = apnea.process(detector.lastDb, detector.noiseFloorDb, apneaTv.isOn || tv.isOn || soundsOn)
                        if (ep != null && System.currentTimeMillis() >= delayUntil) {
                            // Extrait : 4 s avant le silence jusqu'à la fin de la reprise
                            val startFi = ((ep.silenceStartMs - 4_000) / 100).coerceAtLeast(fi - ring.size + 1)
                            val parts = ring.toList().takeLast((fi - startFi + 1).toInt().coerceAtMost(ring.size))
                            val clip = ShortArray(parts.sumOf { it.size }).also { out -> var p = 0; parts.forEach { a -> a.copyInto(out, p); p += a.size } }
                            val clipRate = if (hi != null) rec.rate else AudioMath.SAMPLE_RATE
                            val ts = startTime + (fi - parts.size + 1) * 100
                            val floorNow = detector.noiseFloorDb
                            launch(Dispatchers.IO) { saveApnea(ep, clip, clipRate, ts, floorNow) }
                        }
                    }
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
                            apneaStatus = if (settings.apneaMode) apnea.status else null,
                        )
                    }
                }
                detector.flush()?.let { events.trySend(it) }
            } finally {
                rec.release()
            }
        }
    }

    private fun startProcessing(c: SoundClassifier) {
        val ec = EventClassifier(c, settings.thresholds())
        val dir = File(filesDir, "recordings/$sessionId").apply { mkdirs() }
        processJob = lifecycleScope.launch(Dispatchers.Default) {
            // Son long découpé toutes les 60 s : les morceaux suivants sont recollés ici
            class OpenChunk(val id: Long, val audio: String?, val enhanced: String?, val gain: Float, val type: SoundType)
            var open: OpenChunk? = null
            for (ev in events) {
                val rate = if (ev.pcmHi != null) ev.hiRate else AudioMath.SAMPLE_RATE
                val chunk = ev.pcmHi ?: ev.pcm
                if (ev.continuation) {
                    val o = open
                    if (o != null) {
                        o.audio?.let { p ->
                            val add = com.somnia.app.audio.Resample.polishWith(chunk, rate, o.gain, fadeIn = false, fadeOut = !ev.continues).first
                            WavFile.write(File(p), WavFile.read(File(p)) + add, rate)
                        }
                        o.enhanced?.let { p -> WavFile.write(File(p), WavFile.read(File(p)) + VoiceEnhancer.enhance(chunk, null, rate), rate) }
                        dao.event(o.id)?.let { e ->
                            var upd = e.copy(durationMs = e.durationMs + ev.durationMs, peakDb = maxOf(e.peakDb, ev.peakDb))
                            if (!ev.continues && (o.type == SoundType.SPEECH || o.type == SoundType.NIGHT_TERROR)) {
                                (o.enhanced ?: o.audio)?.let { path ->
                                    runCatching { com.somnia.app.transcribe.Transcriber.transcribe(this@SleepTrackingService, path) }
                                        .getOrNull()?.let { upd = upd.copy(transcript = it) }
                                }
                            }
                            dao.updateEvent(upd)
                        }
                    }
                    if (!ev.continues) open = null
                    continue // la suite d'un son ignoré est ignorée aussi
                }
                open = null
                val cls = ec.classify(ev.pcm)
                val ts = startTime + ev.startOffsetMs
                val keepAudio = cls.type in settings.recordTypes
                if (cls.type == SoundType.MEDIA && SoundType.MEDIA !in settings.recordTypes) {
                    TrackingState.live.value = TrackingState.live.value?.let { it.copy(ignoredMedia = it.ignoredMedia + 1) }
                    continue // télé / musique : bruit de fond
                }
                if (cls.type == SoundType.NOISE && SoundType.NOISE !in settings.recordTypes) {
                    synchronized(lock) { minuteEvents++ }
                    continue // bruit non identifié : on ne garde rien
                }
                // Paroles : on ne garde que VOTRE voix (si l'empreinte vocale est faite)
                if (cls.type == SoundType.SPEECH && settings.onlyMyVoice) {
                    val print = com.somnia.app.transcribe.VoiceId.voiceprint(this@SleepTrackingService)
                    if (print != null) {
                        val v = runCatching { com.somnia.app.transcribe.VoiceId.vector(this@SleepTrackingService, ev.pcm) }.getOrNull()
                        if (v != null && com.somnia.app.transcribe.VoiceId.cosine(print, v) < settings.voiceMatch) {
                            TrackingState.live.value = TrackingState.live.value?.let { it.copy(ignoredVoices = it.ignoredVoices + 1) }
                            continue // une autre voix (télé, radio, autre personne)
                        }
                    }
                }
                var audio: String? = null
                var enhanced: String? = null
                var gain = 1f
                if (keepAudio) {
                    val f = File(dir, "${ts}_${cls.type.name.lowercase()}.wav")
                    // Original en haute qualité, légèrement retouché (grondement, volume, clics)
                    val (polished, g) = com.somnia.app.audio.Resample.polishWith(chunk, rate, fadeOut = !ev.continues)
                    WavFile.write(f, polished, rate)
                    gain = g
                    audio = f.absolutePath
                    val voiceLike = cls.type in setOf(SoundType.SPEECH, SoundType.NIGHT_TERROR, SoundType.LAUGHTER, SoundType.CRYING)
                    if (settings.enhanceVoice && voiceLike) {
                        val e = File(dir, "${ts}_${cls.type.name.lowercase()}_voix.wav")
                        if (ev.pcmHi != null) WavFile.write(e, VoiceEnhancer.enhance(ev.pcmHi, null, ev.hiRate), ev.hiRate)
                        else WavFile.write(e, VoiceEnhancer.enhance(ev.pcm, ev.noiseSample))
                        enhanced = e.absolutePath
                    }
                }
                val eventId = dao.insertEvent(
                    SoundEventEntity(
                        sessionId = sessionId, timestamp = ts, durationMs = ev.durationMs,
                        type = cls.type.name, confidence = cls.confidence, peakDb = ev.peakDb,
                        noiseFloorDb = ev.noiseFloorDb,
                        scores = cls.scores.entries.sortedByDescending { it.value }
                            .joinToString(";") { "${it.key.name}=${"%.2f".format(java.util.Locale.US, it.value)}" },
                        audioPath = audio, enhancedPath = enhanced,
                        // Type désactivé dans les réglages : compté, mais ni enregistré ni affiché
                        hidden = !keepAudio,
                    ),
                )
                if (ev.continues) open = OpenChunk(eventId, audio, enhanced, gain, cls.type)
                // Paroles et cris : transcription en texte si le modèle est installé (à la fin d'un son long)
                if (!ev.continues && (cls.type == SoundType.SPEECH || cls.type == SoundType.NIGHT_TERROR)) {
                    (enhanced ?: audio)?.let { path ->
                        runCatching { com.somnia.app.transcribe.Transcriber.transcribe(this@SleepTrackingService, path) }
                            .getOrNull()?.let { text -> dao.event(eventId)?.let { dao.updateEvent(it.copy(transcript = text)) } }
                    }
                }
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

    /** Enregistre une pause respiratoire suspecte (son + ligne dans la nuit). */
    private suspend fun saveApnea(ep: com.somnia.app.audio.ApneaDetector.Episode, clip: ShortArray, rate: Int, ts: Long, floorDb: Float) {
        val dir = File(filesDir, "recordings/$sessionId").apply { mkdirs() }
        val f = File(dir, "${ts}_apnee.wav")
        WavFile.write(f, com.somnia.app.audio.Resample.polish(clip, rate), rate)
        dao.insertEvent(
            SoundEventEntity(
                sessionId = sessionId, timestamp = ts, durationMs = clip.size * 1000L / rate,
                type = SoundType.APNEA.name, confidence = 1f, peakDb = ep.recoveryPeakDb, noiseFloorDb = floorDb,
                scores = "APNEA_GAP=${"%.1f".format(java.util.Locale.US, ep.gapMs / 1000f)}",
                audioPath = f.absolutePath, enhancedPath = null,
            ),
        )
        TrackingState.live.value = TrackingState.live.value?.let { it.copy(apneaCount = it.apneaCount + 1) }
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
                // inclinaison moyenne du téléphone (positions de sommeil)
                tilt = if (gCount > 0) Math.toDegrees(kotlin.math.atan2(gSum[0], gSum[2])).toFloat() else 0f,
                pitch = if (gCount > 0) Math.toDegrees(kotlin.math.atan2(gSum[1], gSum[2])).toFloat() else 0f,
                hasPosition = gCount > 0,
            )
            minuteDbSum = 0.0; minuteDbCount = 0; minuteDbMax = -96f; minuteMovement = 0f; minuteEvents = 0
            gSum.fill(0.0); gCount = 0
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
        synchronized(lock) {
            if (mag > 0.05f) minuteMovement += (mag - 0.05f)
            for (i in 0..2) gSum[i] += gravity[i]
            gCount++
        }
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
            com.somnia.app.widget.DodoWidget.refresh(this@SleepTrackingService)
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
        // Calibration : activité habituelle apprise sur les nuits précédentes
        val past = dao.recentMovementMedians(id)
        val baseline = if (past.size >= 3) past.sorted()[past.size / 2] else null
        val nightMedian = minutes.map { it.movement }.sorted().let { if (it.isEmpty()) null else it[it.size / 2] }
        val a = SleepAnalyzer.analyze(input, settings.sleepGoalMin, baseline)
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
                movementMedian = if (settings.trackMovement) nightMedian else null,
                calibrationNights = if (baseline != null) past.size else 0,
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
            .setContentTitle("Police du Dodo surveille votre nuit")
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
