package com.somnia.app.transcribe

import android.content.Context
import com.somnia.app.audio.WavFile
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.withContext
import org.json.JSONObject
import org.vosk.Model
import org.vosk.Recognizer
import org.vosk.SpeakerModel
import java.io.File
import java.net.URL
import java.util.zip.ZipInputStream
import kotlin.math.sqrt

/**
 * Reconnaissance de VOTRE voix (empreinte vocale), sur le téléphone.
 *
 * On enregistre ~20 s de votre voix une fois ; Vosk en tire un vecteur
 * (« x-vector ») qui résume le timbre. La nuit, chaque parole est comparée
 * à cette empreinte : une voix de télé ou d'une autre personne ne colle pas.
 * Nécessite le modèle français (transcription) + le modèle « locuteur » (14 Mo).
 */
object VoiceId {
    private const val SPK_URL = "https://alphacephei.com/vosk/models/vosk-model-spk-0.4.zip"
    private const val SPK_NAME = "vosk-model-spk-0.4"

    private val _downloading = MutableStateFlow<Float?>(null)
    val downloading: StateFlow<Float?> = _downloading
    @Volatile private var spk: SpeakerModel? = null
    @Volatile private var model: Model? = null

    private fun dir(ctx: Context) = File(ctx.filesDir, "voix")
    private fun spkDir(ctx: Context) = File(dir(ctx), SPK_NAME)
    private fun prefs(ctx: Context) = ctx.getSharedPreferences("voix", Context.MODE_PRIVATE)

    fun isModelReady(ctx: Context) = File(spkDir(ctx), "final.ext.raw").exists() || File(spkDir(ctx), "mfcc.conf").exists()
    fun isReady(ctx: Context) = isModelReady(ctx) && Transcriber.isReady(ctx)
    fun hasVoiceprint(ctx: Context) = voiceprint(ctx) != null

    suspend fun downloadModel(ctx: Context) = withContext(Dispatchers.IO) {
        _downloading.value = 0f
        try {
            val tmp = File(ctx.cacheDir, "spk.zip")
            val conn = URL(SPK_URL).openConnection().apply { connectTimeout = 20_000; readTimeout = 30_000 }
            val total = conn.contentLengthLong.takeIf { it > 0 } ?: 13_869_103L
            conn.getInputStream().use { input ->
                tmp.outputStream().use { out ->
                    val buf = ByteArray(64 * 1024); var done = 0L
                    while (true) {
                        val n = input.read(buf); if (n < 0) break
                        out.write(buf, 0, n); done += n
                        _downloading.value = (done.toFloat() / total).coerceIn(0f, 0.97f)
                    }
                }
            }
            val target = dir(ctx).apply { deleteRecursively(); mkdirs() }
            ZipInputStream(tmp.inputStream().buffered()).use { zip ->
                while (true) {
                    val e = zip.nextEntry ?: break
                    val f = File(target, e.name)
                    if (!f.canonicalPath.startsWith(target.canonicalPath)) continue
                    if (e.isDirectory) f.mkdirs() else { f.parentFile?.mkdirs(); f.outputStream().use { zip.copyTo(it) } }
                }
            }
            tmp.delete()
        } finally {
            _downloading.value = null
        }
    }

    /** Vecteur de voix d'un enregistrement (null si trop peu de parole). */
    suspend fun vector(ctx: Context, pcm: ShortArray): FloatArray? = withContext(Dispatchers.Default) {
        if (!isReady(ctx)) return@withContext null
        val (m, s) = synchronized(this@VoiceId) {
            val m = model ?: Transcriber.loadModel(ctx).also { model = it }
            val s = spk ?: SpeakerModel(spkDir(ctx).absolutePath).also { spk = it }
            m to s
        }
        Recognizer(m, 16000f, s).use { rec ->
            var i = 0
            while (i < pcm.size) {
                val n = minOf(4000, pcm.size - i); rec.acceptWaveForm(pcm.copyOfRange(i, i + n), n); i += n
            }
            val json = JSONObject(rec.finalResult)
            val arr = json.optJSONArray("spk") ?: return@use null
            if (json.optInt("spk_frames", 0) < 60) return@use null // moins d'~0,6 s de voix : pas fiable
            FloatArray(arr.length()) { arr.getDouble(it).toFloat() }
        }
    }

    suspend fun vector(ctx: Context, wavPath: String): FloatArray? = vector(ctx, WavFile.read16k(File(wavPath)))

    /** Enregistre l'empreinte à partir de ~20 s de voix. */
    suspend fun enroll(ctx: Context, pcm: ShortArray): Boolean {
        // moyenne de plusieurs morceaux pour une empreinte plus stable
        val chunk = 16000 * 5
        val vecs = (0 until pcm.size step chunk).mapNotNull { st ->
            vector(ctx, pcm.copyOfRange(st, minOf(pcm.size, st + chunk)))
        }.ifEmpty { listOfNotNull(vector(ctx, pcm)) }
        if (vecs.isEmpty()) return false
        val mean = FloatArray(vecs[0].size) { k -> vecs.map { normalize(it)[k] }.average().toFloat() }
        prefs(ctx).edit().putString("empreinte", mean.joinToString(",")).apply()
        return true
    }

    fun forget(ctx: Context) = prefs(ctx).edit().remove("empreinte").apply()

    fun voiceprint(ctx: Context): FloatArray? =
        prefs(ctx).getString("empreinte", null)?.split(",")?.mapNotNull { it.toFloatOrNull() }?.toFloatArray()?.takeIf { it.isNotEmpty() }

    /** Ressemblance 0..1 entre la voix d'un enregistrement et l'empreinte (null = indéterminé). */
    suspend fun similarity(ctx: Context, wavPath: String): Float? {
        val print = voiceprint(ctx) ?: return null
        val v = vector(ctx, wavPath) ?: return null
        return cosine(print, v)
    }

    fun cosine(a: FloatArray, b: FloatArray): Float {
        if (a.size != b.size) return 0f
        var dot = 0.0; var na = 0.0; var nb = 0.0
        for (i in a.indices) { dot += a[i] * b[i]; na += a[i] * a[i]; nb += b[i] * b[i] }
        return if (na == 0.0 || nb == 0.0) 0f else (dot / (sqrt(na) * sqrt(nb))).toFloat()
    }

    private fun normalize(v: FloatArray): FloatArray {
        var n = 0.0; for (x in v) n += x * x
        val s = sqrt(n).toFloat().takeIf { it > 0 } ?: 1f
        return FloatArray(v.size) { v[it] / s }
    }
}
