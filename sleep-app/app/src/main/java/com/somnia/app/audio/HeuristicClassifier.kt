package com.somnia.app.audio

import kotlin.math.abs
import kotlin.math.log10
import kotlin.math.max
import kotlin.math.sqrt

/**
 * Classifieur de secours (si le modèle IA n'a pas pu être chargé) basé sur
 * des mesures acoustiques simples : hauteur de la voix (autocorrélation),
 * part des basses fréquences, centre de gravité spectral, rythme des
 * syllabes. Moins précis que YAMNet, mais sans dépendance.
 */
class HeuristicClassifier : SoundClassifier {
    override val name = "Analyse acoustique simple"
    private val fft = Fft(512)

    data class Features(
        val db: Float,
        val voicedRatio: Float,
        val meanPitch: Float,
        val lowRatio: Float,
        val centroid: Float,
        val burstRate: Float,
    )

    fun features(w: FloatArray): Features {
        val sr = AudioMath.SAMPLE_RATE
        // --- Hauteur (sous-trames de 32 ms) ---
        val sub = 512
        var voiced = 0; var frames = 0; var pitchSum = 0f
        var i = 0
        while (i + sub <= w.size) {
            var e0 = 0.0
            for (k in 0 until sub) e0 += w[i + k] * w[i + k]
            if (e0 > 1e-6) {
                var best = 0.0; var bestLag = 0
                for (lag in sr / 500..sr / 70) {
                    var acc = 0.0
                    for (k in 0 until sub - lag) acc += w[i + k] * w[i + k + lag]
                    if (acc > best) { best = acc; bestLag = lag }
                }
                if (best / e0 > 0.45 && bestLag > 0) { voiced++; pitchSum += sr.toFloat() / bestLag }
            }
            frames++
            i += sub
        }
        // --- Spectre moyen ---
        val re = DoubleArray(512); val im = DoubleArray(512)
        val power = DoubleArray(257)
        i = 0
        while (i + 512 <= w.size) {
            for (k in 0 until 512) { re[k] = w[i + k].toDouble(); im[k] = 0.0 }
            fft.transform(re, im)
            for (k in 0..256) power[k] += re[k] * re[k] + im[k] * im[k]
            i += 512
        }
        val total = power.sum().coerceAtLeast(1e-12)
        val binHz = sr / 512f
        var low = 0.0; var cen = 0.0
        for (k in 0..256) { if (k * binHz < 300) low += power[k]; cen += k * binHz * power[k] }
        // --- Rythme des bouffées d'énergie (syllabes / éclats de rire) ---
        val env = 480 // 30 ms
        val envs = FloatArray(w.size / env) { idx ->
            var s = 0f; for (k in 0 until env) s += abs(w[idx * env + k]); s / env
        }
        val meanEnv = envs.average().toFloat()
        var bursts = 0
        for (k in 1 until envs.size - 1) {
            if (envs[k] > meanEnv * 1.3f && envs[k] >= envs[k - 1] && envs[k] > envs[k + 1]) bursts++
        }
        var sq = 0.0; for (v in w) sq += v * v
        return Features(
            db = (20 * log10(max(sqrt(sq / w.size), 1e-5))).toFloat(),
            voicedRatio = if (frames == 0) 0f else voiced.toFloat() / frames,
            meanPitch = if (voiced == 0) 0f else pitchSum / voiced,
            lowRatio = (low / total).toFloat(),
            centroid = (cen / total).toFloat(),
            burstRate = bursts / (w.size.toFloat() / sr),
        )
    }

    override fun classify(window: FloatArray): SoundScores {
        val f = features(window)
        fun ramp(x: Float, a: Float, b: Float) = ((x - a) / (b - a)).coerceIn(0f, 1f)
        val voiced = f.voicedRatio
        val scores = mutableMapOf<SoundType, Float>()
        scores[SoundType.SNORING] = 0.7f * ramp(f.lowRatio, 0.35f, 0.75f) * ramp(600f - f.centroid, 0f, 300f)
        scores[SoundType.SPEECH] = 0.6f * ramp(voiced, 0.2f, 0.5f) *
            (if (f.meanPitch in 80f..320f) 1f else 0.4f) * ramp(f.burstRate, 1.5f, 3.5f)
        scores[SoundType.LAUGHTER] = 0.55f * ramp(voiced, 0.2f, 0.5f) *
            ramp(f.burstRate, 3.5f, 6f) * ramp(f.meanPitch, 160f, 260f)
        scores[SoundType.CRYING] = 0.55f * ramp(voiced, 0.5f, 0.8f) *
            ramp(f.meanPitch, 280f, 420f) * ramp(4f - f.burstRate, 0f, 2f)
        scores[SoundType.NIGHT_TERROR] = 0.6f * ramp(voiced, 0.4f, 0.7f) *
            ramp(f.meanPitch, 300f, 500f) * ramp(f.db, -30f, -15f)
        scores[SoundType.COUGH] = 0.5f * ramp(0.25f - voiced, 0f, 0.2f) *
            ramp(f.centroid, 800f, 2000f) * ramp(f.burstRate, 0.5f, 1.5f)
        scores[SoundType.BREATHING] = 0.4f * ramp(0.2f - voiced, 0f, 0.15f) * ramp(f.centroid, 300f, 900f)
        return scores
    }
}
