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
import java.io.File
import java.net.URL
import java.util.zip.ZipInputStream

/**
 * Transcription des paroles en texte, entièrement sur le téléphone (Vosk).
 * Le modèle français (~42 Mo) est téléchargé une seule fois, à la demande.
 */
object Transcriber {
    private const val MODEL_URL = "https://alphacephei.com/vosk/models/vosk-model-small-fr-0.22.zip"
    private const val MODEL_NAME = "vosk-model-small-fr-0.22"

    sealed interface State {
        data object Missing : State
        data class Downloading(val progress: Float) : State
        data object Ready : State
        data class Failed(val message: String) : State
    }

    private val _state = MutableStateFlow<State>(State.Missing)
    val state: StateFlow<State> = _state
    @Volatile private var model: Model? = null

    private fun dir(ctx: Context) = File(ctx.filesDir, "transcription")
    private fun modelDir(ctx: Context) = File(dir(ctx), MODEL_NAME)

    fun isReady(ctx: Context) = File(modelDir(ctx), "am").exists() || File(modelDir(ctx), "conf").exists()

    fun refresh(ctx: Context) {
        if (_state.value !is State.Downloading) _state.value = if (isReady(ctx)) State.Ready else State.Missing
    }

    /** Télécharge et décompresse le modèle français. */
    suspend fun download(ctx: Context) = withContext(Dispatchers.IO) {
        _state.value = State.Downloading(0f)
        try {
            val tmp = File(ctx.cacheDir, "vosk-fr.zip")
            val conn = URL(MODEL_URL).openConnection().apply { connectTimeout = 20_000; readTimeout = 30_000 }
            val total = conn.contentLengthLong.takeIf { it > 0 } ?: 42_233_323L
            conn.getInputStream().use { input ->
                tmp.outputStream().use { out ->
                    val buf = ByteArray(64 * 1024)
                    var done = 0L
                    while (true) {
                        val n = input.read(buf); if (n < 0) break
                        out.write(buf, 0, n); done += n
                        _state.value = State.Downloading((done.toFloat() / total).coerceIn(0f, 0.97f))
                    }
                }
            }
            val target = dir(ctx).apply { deleteRecursively(); mkdirs() }
            ZipInputStream(tmp.inputStream().buffered()).use { zip ->
                while (true) {
                    val e = zip.nextEntry ?: break
                    val f = File(target, e.name)
                    if (!f.canonicalPath.startsWith(target.canonicalPath)) continue // sécurité
                    if (e.isDirectory) f.mkdirs() else { f.parentFile?.mkdirs(); f.outputStream().use { zip.copyTo(it) } }
                }
            }
            tmp.delete()
            _state.value = if (isReady(ctx)) State.Ready else State.Failed("Modèle incomplet")
        } catch (e: Exception) {
            _state.value = State.Failed("Téléchargement impossible : vérifiez la connexion Internet.")
        }
    }

    fun delete(ctx: Context) {
        synchronized(this) { model?.close(); model = null }
        dir(ctx).deleteRecursively()
        _state.value = State.Missing
    }

    /** Modèle de langue partagé (transcription + empreinte vocale). */
    fun loadModel(ctx: Context): Model = synchronized(this) { model ?: Model(modelDir(ctx).absolutePath).also { model = it } }

    /** Texte reconnu dans un fichier WAV 16 kHz mono, ou null si rien / modèle absent. */
    suspend fun transcribe(ctx: Context, wavPath: String): String? = withContext(Dispatchers.Default) {
        if (!isReady(ctx)) return@withContext null
        val m = loadModel(ctx)
        val pcm = WavFile.read(File(wavPath))
        Recognizer(m, 16000f).use { rec ->
            var i = 0
            while (i < pcm.size) {
                val n = minOf(4000, pcm.size - i)
                rec.acceptWaveForm(pcm.copyOfRange(i, i + n), n)
                i += n
            }
            JSONObject(rec.finalResult).optString("text").trim().takeIf { it.isNotEmpty() }
                ?.replaceFirstChar { it.uppercase() }
        }
    }
}
