package com.somnia.app.audio

import android.media.AudioAttributes
import android.media.AudioFormat
import android.media.AudioTrack
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlin.math.PI
import kotlin.math.exp
import kotlin.math.sin
import kotlin.random.Random

/** Ambiances sonores fabriquées par l'appli (aucun fichier audio). */
enum class Ambience(val label: String) {
    RAIN("Pluie"), WAVES("Vagues"), WHITE("Bruit blanc"), PINK("Bruit rose"), BROWN("Bruit brun");

    companion object { fun fromName(n: String) = entries.firstOrNull { it.name == n } ?: RAIN }
}

/** Générateur d'échantillons (-1..1) pour une ambiance, à [rate] Hz. */
class AmbienceGenerator(private val type: Ambience, private val rate: Int = 22_050, seed: Int = 1) {
    private val rnd = Random(seed)
    private var b0 = 0.0; private var b1 = 0.0; private var b2 = 0.0; private var b3 = 0.0; private var b4 = 0.0; private var b5 = 0.0; private var b6 = 0.0
    private var brown = 0.0
    private var n = 0L
    private var drop = 0.0; private var dropDecay = 0.0; private var dropPhase = 0.0; private var dropFreq = 0.0

    private fun white() = rnd.nextDouble(-1.0, 1.0)

    /** Bruit rose (filtre de Paul Kellet). */
    private fun pink(): Double {
        val w = white()
        b0 = 0.99886 * b0 + w * 0.0555179; b1 = 0.99332 * b1 + w * 0.0750759; b2 = 0.96900 * b2 + w * 0.1538520
        b3 = 0.86650 * b3 + w * 0.3104856; b4 = 0.55000 * b4 + w * 0.5329522; b5 = -0.7616 * b5 - w * 0.0168980
        val p = b0 + b1 + b2 + b3 + b4 + b5 + b6 + w * 0.5362
        b6 = w * 0.115926
        return p * 0.11
    }

    private fun brownNoise(): Double { brown = (brown + 0.02 * white()) / 1.02; return brown * 3.5 }

    fun next(): Float {
        n++
        val t = n.toDouble() / rate
        val v = when (type) {
            Ambience.WHITE -> white() * 0.22
            Ambience.PINK -> pink() * 0.9
            Ambience.BROWN -> brownNoise() * 0.8
            Ambience.WAVES -> {
                // houle lente (~9 s) + écume
                val swell = 0.5 + 0.5 * sin(2 * PI * t / 9.0 + 0.6 * sin(2 * PI * t / 23.0))
                brownNoise() * (0.15 + 0.85 * swell * swell) * 0.9 + pink() * 0.35 * swell * swell * swell
            }
            Ambience.RAIN -> {
                // fond de pluie + gouttes
                if (rnd.nextDouble() < 40.0 / rate) {
                    drop = rnd.nextDouble(0.05, 0.25); dropDecay = exp(-1.0 / (rate * rnd.nextDouble(0.004, 0.015)))
                    dropFreq = rnd.nextDouble(1_500.0, 5_000.0); dropPhase = 0.0
                }
                dropPhase += 2 * PI * dropFreq / rate
                val d = drop * sin(dropPhase); drop *= dropDecay
                pink() * 0.55 + d
            }
        }
        return v.toFloat().coerceIn(-1f, 1f)
    }
}

/**
 * Lecteur d'ambiance : joue en boucle avec un volume qui suit [volumeAt]
 * (ms écoulées -> 0..1 ; une valeur négative arrête le son).
 */
class AmbiencePlayer(private val type: Ambience, private val alarm: Boolean, private val volumeAt: (Long) -> Float) {
    @Volatile private var running = false
    private var thread: Thread? = null

    fun start() {
        running = true
        thread = Thread {
            val rate = 22_050
            val min = AudioTrack.getMinBufferSize(rate, AudioFormat.CHANNEL_OUT_MONO, AudioFormat.ENCODING_PCM_16BIT)
            val track = AudioTrack.Builder()
                .setAudioAttributes(
                    AudioAttributes.Builder()
                        .setUsage(if (alarm) AudioAttributes.USAGE_ALARM else AudioAttributes.USAGE_MEDIA)
                        .setContentType(AudioAttributes.CONTENT_TYPE_MUSIC).build(),
                )
                .setAudioFormat(AudioFormat.Builder().setSampleRate(rate).setEncoding(AudioFormat.ENCODING_PCM_16BIT).setChannelMask(AudioFormat.CHANNEL_OUT_MONO).build())
                .setBufferSizeInBytes(maxOf(min, 8192)).setTransferMode(AudioTrack.MODE_STREAM).build()
            val gen = AmbienceGenerator(type, rate)
            val buf = ShortArray(2048)
            val start = System.currentTimeMillis()
            runCatching {
                track.play()
                while (running) {
                    val vol = volumeAt(System.currentTimeMillis() - start)
                    if (vol < 0f) break
                    for (i in buf.indices) buf[i] = (gen.next() * vol * 30_000).toInt().toShort()
                    track.write(buf, 0, buf.size)
                }
            }
            runCatching { track.stop() }; track.release()
            running = false
        }.apply { isDaemon = true; start() }
    }

    fun stop() { running = false }
    val isRunning get() = running
}

/** Sons pour s'endormir, avec arrêt progressif. Pendant la lecture, la détection est mise en pause. */
object SleepSounds {
    data class Active(val type: Ambience, val endsAt: Long)

    private val _active = MutableStateFlow<Active?>(null)
    val active: StateFlow<Active?> = _active
    @Volatile private var lastStop = 0L
    private var player: AmbiencePlayer? = null

    fun play(type: Ambience, minutes: Int) {
        stop()
        val total = minutes * 60_000L
        val fade = minOf(3 * 60_000L, total / 3) // baisse progressive sur les 3 dernières minutes
        _active.value = Active(type, System.currentTimeMillis() + total)
        player = AmbiencePlayer(type, alarm = false) { ms ->
            when {
                ms >= total -> -1f
                ms < 5_000 -> 0.6f * ms / 5_000f
                ms > total - fade -> 0.6f * (total - ms).toFloat() / fade
                else -> 0.6f
            }
        }.also { it.start() }
        // fin automatique
        Thread { Thread.sleep(total + 500); if (_active.value?.endsAt?.let { it <= System.currentTimeMillis() + 1_000 } == true) stop() }
            .apply { isDaemon = true; start() }
    }

    fun stop() {
        player?.stop(); player = null
        if (_active.value != null) lastStop = System.currentTimeMillis()
        _active.value = null
    }

    /** Vrai pendant la lecture et quelques secondes après (le temps que le son s'éteigne). */
    fun activeOrRecent(graceMs: Long = 8_000): Boolean =
        _active.value != null || System.currentTimeMillis() - lastStop < graceMs
}

/** Sonnerie du réveil qui monte doucement (sonnerie du téléphone ou ambiance). */
object AlarmRinger {
    private var player: AmbiencePlayer? = null
    private var media: android.media.MediaPlayer? = null
    private val handler by lazy { android.os.Handler(android.os.Looper.getMainLooper()) }

    /** Volume progressif : de 5 % à 100 % en 60 s. */
    fun ramp(ms: Long): Float = (0.05f + 0.95f * (ms / 60_000f)).coerceIn(0.05f, 1f)

    fun start(context: android.content.Context, sound: String) {
        stop()
        if (sound == "ringtone") {
            val uri = android.media.RingtoneManager.getDefaultUri(android.media.RingtoneManager.TYPE_ALARM)
                ?: android.media.RingtoneManager.getDefaultUri(android.media.RingtoneManager.TYPE_RINGTONE) ?: return
            media = runCatching {
                android.media.MediaPlayer().apply {
                    setAudioAttributes(AudioAttributes.Builder().setUsage(AudioAttributes.USAGE_ALARM).setContentType(AudioAttributes.CONTENT_TYPE_SONIFICATION).build())
                    setDataSource(context, uri); isLooping = true; prepare(); setVolume(0.05f, 0.05f); start()
                }
            }.getOrNull()
            val start = System.currentTimeMillis()
            handler.post(object : Runnable {
                override fun run() {
                    val m = media ?: return
                    val v = ramp(System.currentTimeMillis() - start); m.setVolume(v, v)
                    if (v < 1f) handler.postDelayed(this, 1_000)
                }
            })
        } else {
            player = AmbiencePlayer(Ambience.fromName(sound), alarm = true) { ms -> ramp(ms) }.also { it.start() }
        }
        // vibration douce après 30 s
        handler.postDelayed({
            if (player != null || media != null) runCatching {
                val vib = context.getSystemService(android.os.Vibrator::class.java)
                vib?.vibrate(android.os.VibrationEffect.createWaveform(longArrayOf(0, 600, 1_400), 0))
            }
        }, 30_000)
    }

    fun stop(context: android.content.Context? = null) {
        if (player != null || media != null) handler.removeCallbacksAndMessages(null)
        player?.stop(); player = null
        media?.runCatching { stop(); release() }; media = null
        context?.getSystemService(android.os.Vibrator::class.java)?.cancel()
    }
}
