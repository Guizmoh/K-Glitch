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

    fun shareAudio(context: Context, path: String) {
        val uri = FileProvider.getUriForFile(context, "${context.packageName}.files", File(path))
        context.startActivity(
            Intent.createChooser(
                Intent(Intent.ACTION_SEND).setType("audio/wav").putExtra(Intent.EXTRA_STREAM, uri)
                    .addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION),
                "Partager l'enregistrement",
            ),
        )
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
