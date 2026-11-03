package com.somnia.app.ui

import android.app.Application
import android.content.Context
import android.content.Intent
import androidx.core.content.FileProvider
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import com.somnia.app.alarm.AlarmScheduler
import com.somnia.app.data.Settings
import com.somnia.app.data.SettingsRepository
import com.somnia.app.data.SleepSession
import com.somnia.app.data.SomniaDatabase
import com.somnia.app.data.SoundEventEntity
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.flow.SharingStarted
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.stateIn
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import java.io.File

class SomniaViewModel(app: Application) : AndroidViewModel(app) {
    val dao = SomniaDatabase.get(app).dao()
    private val settingsRepo = SettingsRepository.get(app)
    val settings: StateFlow<Settings> = settingsRepo.settings
    val sessions: StateFlow<List<SleepSession>> =
        dao.sessions().stateIn(viewModelScope, SharingStarted.Eagerly, emptyList())

    /** Étiquettes du journal choisies avant de lancer la nuit. */
    var pendingTags: Set<String> = emptySet()

    fun updateSettings(t: (Settings) -> Settings) {
        settingsRepo.update(t)
        AlarmScheduler.scheduleBedtime(getApplication(), settingsRepo.settings.value)
    }

    fun saveSession(s: SleepSession) = viewModelScope.launch { dao.updateSession(s) }

    fun applyPendingTags(sessionId: Long) = viewModelScope.launch {
        if (pendingTags.isEmpty()) return@launch
        dao.session(sessionId)?.let { if (it.tags.isEmpty()) dao.updateSession(it.copy(tags = pendingTags.joinToString(","))) }
    }

    /** Corrige le type d'un son et ajuste la sensibilité ; renvoie le message à afficher. */
    fun correctEvent(e: SoundEventEntity, right: com.somnia.app.audio.SoundType): String? {
        val wrong = com.somnia.app.audio.SoundType.fromName(e.type)
        viewModelScope.launch { dao.updateEvent(e.copy(type = right.name, userCorrected = true)) }
        var msg: String? = null
        settingsRepo.update { st -> com.somnia.app.analysis.Feedback.adjust(st, wrong, right).also { msg = it.second }.first }
        return msg
    }

    fun transcribe(e: SoundEventEntity) = viewModelScope.launch {
        val path = e.enhancedPath ?: e.audioPath ?: return@launch
        val text = com.somnia.app.transcribe.Transcriber.transcribe(getApplication(), path) ?: "(rien de compréhensible)"
        dao.event(e.id)?.let { dao.updateEvent(it.copy(transcript = text)) }
    }

    fun toggleNightFavorite(s: SleepSession) = viewModelScope.launch { dao.updateSession(s.copy(favorite = !s.favorite)) }

    fun toggleFavorite(e: SoundEventEntity) = viewModelScope.launch { dao.updateEvent(e.copy(favorite = !e.favorite)) }

    fun deleteEvent(e: SoundEventEntity) = viewModelScope.launch {
        AudioPlayer.stop()
        e.audioPath?.let { File(it).delete() }
        e.enhancedPath?.let { File(it).delete() }
        dao.deleteEvent(e)
    }

    fun deleteSession(s: SleepSession) = viewModelScope.launch {
        AudioPlayer.stop()
        File(getApplication<Application>().filesDir, "recordings/${s.id}").deleteRecursively()
        dao.deleteSession(s)
    }

    /**
     * Partage un son (mail, WhatsApp, MMS…). Le fichier est copié avec un nom lisible,
     * et le texte transcrit (s'il existe) est joint au message.
     */
    fun shareAudio(context: Context, e: SoundEventEntity, path: String, enhanced: Boolean) {
        val type = com.somnia.app.audio.SoundType.fromName(e.type)
        val name = "Police-du-Dodo_${type.label.substringBefore(' ').replace("/", "")}_${Fmt.time(e.timestamp).replace(":", "h")}" +
            (if (enhanced) "_voix-claire" else "") + ".wav"
        val dir = File(context.cacheDir, "exports").apply { mkdirs() }
        val copy = File(dir, name).also { File(path).copyTo(it, overwrite = true) }
        val uri = FileProvider.getUriForFile(context, "${context.packageName}.files", copy)
        val text = buildString {
            append("${type.label} à ${Fmt.time(e.timestamp)} (${Fmt.seconds(e.durationMs)}) – Police du Dodo")
            e.transcript?.let { append("\n« $it »") }
        }
        val send = Intent(Intent.ACTION_SEND).setType("audio/wav")
            .putExtra(Intent.EXTRA_STREAM, uri).putExtra(Intent.EXTRA_TEXT, text).putExtra(Intent.EXTRA_SUBJECT, "Enregistrement de la nuit")
            .addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
        send.clipData = android.content.ClipData.newRawUri(name, uri)
        context.startActivity(Intent.createChooser(send, "Partager par mail, WhatsApp, MMS…").addFlags(Intent.FLAG_ACTIVITY_NEW_TASK))
    }

    /** Export CSV de toutes les nuits et de tous les événements. */
    fun exportCsv(context: Context) = viewModelScope.launch {
        val file = withContext(Dispatchers.IO) {
            val dir = File(context.cacheDir, "exports").apply { mkdirs() }
            val f = File(dir, "police_du_dodo_export.csv")
            val sessions = dao.finishedSessions()
            val events = dao.allEvents().groupBy { it.sessionId }
            f.bufferedWriter().use { w ->
                w.appendLine("nuit_debut;nuit_fin;score;profond_min;leger_min;eveil_min;reveils;ronflement_min;humeur;etiquettes;evenement_heure;evenement_type;duree_s;confiance;pic_db")
                for (s in sessions) {
                    val base = listOf(
                        Fmt.day(s.startTime) + " " + Fmt.time(s.startTime), s.endTime?.let { Fmt.time(it) } ?: "",
                        s.score?.toString() ?: "", s.deepMinutes, s.lightMinutes, s.awakeMinutes, s.awakenings,
                        s.snoringMinutes, s.morningMood, s.tags,
                    ).joinToString(";")
                    val evs = events[s.id].orEmpty()
                    if (evs.isEmpty()) w.appendLine("$base;;;;;")
                    for (e in evs) {
                        w.appendLine("$base;${Fmt.timeSec(e.timestamp)};${e.type};${e.durationMs / 1000};${"%.2f".format(e.confidence)};${"%.1f".format(e.peakDb)}")
                    }
                }
            }
            f
        }
        val uri = FileProvider.getUriForFile(context, "${context.packageName}.files", file)
        context.startActivity(
            Intent.createChooser(
                Intent(Intent.ACTION_SEND).setType("text/csv").putExtra(Intent.EXTRA_STREAM, uri)
                    .addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION),
                "Exporter mes nuits",
            ),
        )
    }
}
