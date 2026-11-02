package com.somnia.app.ui.screens

import androidx.compose.foundation.horizontalScroll
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.ExperimentalLayoutApi
import androidx.compose.foundation.layout.FlowRow
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.rememberScrollState
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.filled.ArrowBack
import androidx.compose.material.icons.filled.Delete
import androidx.compose.material.icons.filled.Favorite
import androidx.compose.material.icons.filled.FavoriteBorder
import androidx.compose.material.icons.filled.Share
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.AssistChip
import androidx.compose.material3.FilterChip
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp
import com.somnia.app.analysis.Insights
import com.somnia.app.analysis.JournalTags
import com.somnia.app.audio.SoundType
import com.somnia.app.data.MinuteStat
import com.somnia.app.data.SoundEventEntity
import com.somnia.app.ui.AudioPlayer
import com.somnia.app.ui.Fmt
import com.somnia.app.ui.SomniaViewModel
import com.somnia.app.ui.components.Hypnogram
import com.somnia.app.ui.components.NoiseCurve
import com.somnia.app.ui.components.ScoreRing
import com.somnia.app.ui.components.SectionCard
import com.somnia.app.ui.theme.color

@OptIn(ExperimentalLayoutApi::class)
@Composable
fun SessionScreen(vm: SomniaViewModel, id: Long, back: () -> Unit) {
    val ctx = LocalContext.current
    val session by remember(id) { vm.dao.sessionFlow(id) }.collectAsState(initial = null)
    val events by remember(id) { vm.dao.events(id) }.collectAsState(initial = emptyList())
    val minutes by remember(id) { vm.dao.minutesFlow(id) }.collectAsState(initial = emptyList<MinuteStat>())
    val settings by vm.settings.collectAsState()
    var filter by remember { mutableStateOf<SoundType?>(null) }
    var confirmDelete by remember { mutableStateOf(false) }
    val s = session ?: run { Text("Chargement…", Modifier.padding(16.dp)); return }
    var notes by remember(s.id) { mutableStateOf(s.notes) }
    LaunchedEffect(notes) {
        kotlinx.coroutines.delay(600)
        if (notes != s.notes) vm.saveSession(s.copy(notes = notes))
    }

    LazyColumn(Modifier.padding(horizontal = 16.dp)) {
        item {
            Row(verticalAlignment = Alignment.CenterVertically) {
                IconButton(onClick = back) { Icon(Icons.AutoMirrored.Filled.ArrowBack, "Retour") }
                Text(Fmt.day(s.startTime), style = MaterialTheme.typography.titleLarge, modifier = Modifier.weight(1f))
                IconButton(onClick = { confirmDelete = true }) { Icon(Icons.Filled.Delete, "Supprimer la nuit") }
            }
        }
        item {
            SectionCard {
                Row(verticalAlignment = Alignment.CenterVertically) {
                    Box(contentAlignment = Alignment.Center) {
                        ScoreRing(s.score, Modifier.size(96.dp))
                        Column(horizontalAlignment = Alignment.CenterHorizontally) {
                            Text(s.score?.toString() ?: "–", style = MaterialTheme.typography.headlineMedium)
                            Text("score", style = MaterialTheme.typography.labelSmall)
                        }
                    }
                    Column(Modifier.padding(start = 16.dp)) {
                        if (s.endTime == null) Text("Nuit en cours…")
                        Text("Au lit : ${Fmt.time(s.startTime)} → ${s.endTime?.let { Fmt.time(it) } ?: "…"}")
                        s.sleepOnset?.let { Text("Endormi·e vers ${Fmt.time(it)} (${(it - s.startTime) / 60_000} min)") }
                        s.wakeTime?.let { Text("Réveil vers ${Fmt.time(it)}") }
                        Text("Sommeil : ${Fmt.duration(s.deepMinutes + s.lightMinutes)}")
                        Text("Profond ${Fmt.duration(s.deepMinutes)} · léger ${Fmt.duration(s.lightMinutes)}", style = MaterialTheme.typography.bodySmall)
                        Text("${s.awakenings} réveil(s) · ${s.snoringMinutes} min de ronflement", style = MaterialTheme.typography.bodySmall)
                    }
                }
                if (s.endTime != null && s.score == null) {
                    Text("Nuit trop courte (< 30 min) pour calculer un score.", style = MaterialTheme.typography.bodySmall)
                }
            }
        }
        item {
            SectionCard("Phases de la nuit (estimation)") {
                Hypnogram(minutes, events, s.startTime)
                Text(
                    "Estimées à partir des mouvements (téléphone sur le matelas) et des sons. Ce n'est pas un examen médical.",
                    style = MaterialTheme.typography.labelSmall, color = MaterialTheme.colorScheme.onSurfaceVariant,
                )
            }
        }
        if (minutes.size > 1) item {
            SectionCard("Bruit dans la chambre") {
                NoiseCurve(minutes)
                s.avgNoiseFloorDb?.let { Text("Bruit de fond moyen : %.0f dBFS".format(it), style = MaterialTheme.typography.bodySmall) }
            }
        }
        item {
            SectionCard("Au réveil") {
                Text("Comment vous sentez-vous ?")
                Row {
                    (1..5).forEach { m ->
                        TextButton(onClick = { vm.saveSession(s.copy(morningMood = m)) }) {
                            Text(listOf("😫", "😕", "😐", "🙂", "😄")[m - 1], style = MaterialTheme.typography.headlineSmall,
                                color = if (s.morningMood == m) MaterialTheme.colorScheme.primary else MaterialTheme.colorScheme.onSurface)
                        }
                    }
                }
                val tags = s.tags.split(",").filter { it.isNotBlank() }.toSet()
                Text("Journal", style = MaterialTheme.typography.bodySmall)
                FlowRow(horizontalArrangement = Arrangement.spacedBy(6.dp)) {
                    JournalTags.ALL.forEach { t ->
                        FilterChip(
                            selected = t in tags,
                            onClick = { vm.saveSession(s.copy(tags = (if (t in tags) tags - t else tags + t).joinToString(","))) },
                            label = { Text(t) },
                        )
                    }
                }
                OutlinedTextField(notes, { notes = it }, Modifier.fillMaxWidth(), label = { Text("Notes, rêves…") })
            }
        }
        val tips = Insights.advice(s, settings.sleepGoalMin)
        if (tips.isNotEmpty()) item {
            SectionCard("Conseils") { tips.forEach { Text("• $it", Modifier.padding(vertical = 2.dp)) } }
        }
        item {
            Text("Sons enregistrés (${events.size})", style = MaterialTheme.typography.titleMedium, modifier = Modifier.padding(top = 12.dp))
            Row(Modifier.horizontalScroll(rememberScrollState())) {
                FilterChip(filter == null, { filter = null }, { Text("Tous") })
                events.map { SoundType.fromName(it.type) }.distinct().forEach { t ->
                    val n = events.count { it.type == t.name }
                    FilterChip(filter == t, { filter = if (filter == t) null else t }, { Text("${t.emoji} $n") }, Modifier.padding(start = 6.dp))
                }
            }
        }
        val shown = events.filter { filter == null || it.type == filter!!.name }
        if (shown.isEmpty()) item { Text("Aucun son marquant 🎉", Modifier.padding(vertical = 8.dp)) }
        items(shown, key = { it.id }) { e -> EventRow(e, vm, onShare = { p -> vm.shareAudio(ctx, p) }) }
    }

    if (confirmDelete) {
        AlertDialog(
            onDismissRequest = { confirmDelete = false },
            title = { Text("Supprimer cette nuit ?") },
            text = { Text("Les enregistrements audio seront aussi effacés.") },
            confirmButton = { TextButton(onClick = { confirmDelete = false; vm.deleteSession(s); back() }) { Text("Supprimer") } },
            dismissButton = { TextButton(onClick = { confirmDelete = false }) { Text("Annuler") } },
        )
    }
}

@Composable
private fun EventRow(e: SoundEventEntity, vm: SomniaViewModel, onShare: (String) -> Unit) {
    val t = SoundType.fromName(e.type)
    SectionCard {
        Row(verticalAlignment = Alignment.CenterVertically) {
            Text(t.emoji, style = MaterialTheme.typography.headlineSmall)
            Column(Modifier.padding(start = 12.dp).weight(1f)) {
                Text(t.label, color = t.color(), style = MaterialTheme.typography.titleSmall)
                Text(
                    "${Fmt.timeSec(e.timestamp)} · ${Fmt.seconds(e.durationMs)} · +%.0f dB au-dessus du fond".format(e.peakDb - e.noiseFloorDb),
                    style = MaterialTheme.typography.bodySmall,
                )
                if (t != SoundType.NOISE) Text("Confiance ${(e.confidence * 100).toInt()} %", style = MaterialTheme.typography.labelSmall)
            }
            IconButton(onClick = { vm.toggleFavorite(e) }) {
                Icon(if (e.favorite) Icons.Filled.Favorite else Icons.Filled.FavoriteBorder, "Garder")
            }
        }
        val playing = AudioPlayer.playingPath
        Row(horizontalArrangement = Arrangement.spacedBy(6.dp), verticalAlignment = Alignment.CenterVertically) {
            e.audioPath?.let { p ->
                AssistChip(onClick = { AudioPlayer.toggle(p) }, label = { Text(if (playing == p) "⏹ Stop" else "▶ Original") })
            }
            e.enhancedPath?.let { p ->
                AssistChip(onClick = { AudioPlayer.toggle(p) }, label = { Text(if (playing == p) "⏹ Stop" else "🗣️ Voix isolée") })
            }
            if (e.audioPath == null) Text("Audio non conservé", style = MaterialTheme.typography.labelSmall)
            Box(Modifier.weight(1f))
            (e.enhancedPath ?: e.audioPath)?.let { p -> IconButton(onClick = { onShare(p) }) { Icon(Icons.Filled.Share, "Partager") } }
            IconButton(onClick = { vm.deleteEvent(e) }) { Icon(Icons.Filled.Delete, "Supprimer") }
        }
    }
}
