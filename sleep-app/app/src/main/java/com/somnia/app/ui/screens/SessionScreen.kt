package com.somnia.app.ui.screens

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.horizontalScroll
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.ExperimentalLayoutApi
import androidx.compose.foundation.layout.FlowRow
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.filled.ArrowBack
import androidx.compose.material.icons.filled.Delete
import androidx.compose.material.icons.filled.Favorite
import androidx.compose.material.icons.filled.FavoriteBorder
import androidx.compose.material.icons.filled.Share
import androidx.compose.material3.AlertDialog
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
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.somnia.app.analysis.Insights
import com.somnia.app.analysis.JournalTags
import com.somnia.app.audio.SoundType
import com.somnia.app.data.MinuteStat
import com.somnia.app.data.SoundEventEntity
import com.somnia.app.ui.AudioPlayer
import com.somnia.app.ui.Fmt
import com.somnia.app.ui.SomniaViewModel
import com.somnia.app.ui.components.ExpandableCard
import com.somnia.app.ui.components.GlassCard
import com.somnia.app.ui.components.Hypnogram
import com.somnia.app.ui.components.NoiseCurve
import com.somnia.app.ui.components.ScoreRing
import com.somnia.app.ui.components.StatTile
import com.somnia.app.ui.components.TileRow
import com.somnia.app.ui.components.ToggleChip
import com.somnia.app.ui.theme.AwakeColor
import com.somnia.app.ui.theme.CardStroke
import com.somnia.app.ui.theme.DeepColor
import com.somnia.app.ui.theme.LightColor
import com.somnia.app.ui.theme.PoliceBlue
import com.somnia.app.ui.theme.TextSoft
import com.somnia.app.ui.theme.color
import com.somnia.app.ui.theme.scoreColor

private val MOODS = listOf("😫", "😕", "😐", "🙂", "😄")

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
    val s = session ?: run { Text("Chargement…", Modifier.padding(20.dp), color = TextSoft); return }
    var notes by remember(s.id) { mutableStateOf(s.notes) }
    LaunchedEffect(notes) {
        kotlinx.coroutines.delay(600)
        if (notes != s.notes) vm.saveSession(s.copy(notes = notes))
    }

    LazyColumn(Modifier.padding(horizontal = 20.dp)) {
        item {
            Row(Modifier.padding(top = 8.dp), verticalAlignment = Alignment.CenterVertically) {
                IconButton(onClick = back) { Icon(Icons.AutoMirrored.Filled.ArrowBack, "Retour") }
                Column(Modifier.weight(1f)) {
                    Text("Rapport de nuit", style = MaterialTheme.typography.labelLarge, color = TextSoft)
                    Text(Fmt.day(s.startTime), style = MaterialTheme.typography.titleLarge)
                }
                IconButton(onClick = { confirmDelete = true }) { Icon(Icons.Filled.Delete, "Supprimer la nuit", tint = TextSoft) }
            }
        }
        // --- Score ---
        item {
            GlassCard {
                Row(verticalAlignment = Alignment.CenterVertically) {
                    Box(contentAlignment = Alignment.Center) {
                        ScoreRing(s.score, Modifier.size(112.dp))
                        Column(horizontalAlignment = Alignment.CenterHorizontally) {
                            Text(s.score?.toString() ?: "–", fontSize = 36.sp, fontWeight = FontWeight.Bold, color = scoreColor(s.score))
                            Text("/ 100", style = MaterialTheme.typography.labelSmall, color = TextSoft)
                        }
                    }
                    Column(Modifier.padding(start = 18.dp)) {
                        Text(
                            when {
                                s.endTime == null -> "Nuit en cours…"
                                s.score == null -> "Nuit trop courte"
                                s.score >= 80 -> "Excellente nuit ✨"
                                s.score >= 60 -> "Bonne nuit"
                                s.score >= 40 -> "Nuit moyenne"
                                else -> "Nuit difficile"
                            },
                            style = MaterialTheme.typography.titleLarge,
                        )
                        Spacer(Modifier.height(4.dp))
                        Text("🛏️ ${Fmt.time(s.startTime)} → ${s.endTime?.let { Fmt.time(it) } ?: "…"}", color = TextSoft)
                        s.sleepOnset?.let { Text("💤 Endormi·e en ${(it - s.startTime) / 60_000} min", color = TextSoft) }
                        s.wakeTime?.let { Text("☀️ Réveil vers ${Fmt.time(it)}", color = TextSoft) }
                    }
                }
            }
        }
        item {
            TileRow {
                StatTile("😴", Fmt.duration(s.deepMinutes + s.lightMinutes), "Sommeil", Modifier.weight(1f))
                StatTile("🌊", Fmt.duration(s.deepMinutes), "Profond", Modifier.weight(1f), accent = Color(0xFF9C9CFF))
            }
            TileRow {
                StatTile("👀", "${s.awakenings}", "Réveils", Modifier.weight(1f), accent = AwakeColor)
                StatTile("🐻", "${s.snoringMinutes} min", "Ronflement", Modifier.weight(1f), accent = SoundType.SNORING.color())
            }
        }
        // --- Phases ---
        item {
            GlassCard(title = "Phases de la nuit", icon = "🌙") {
                Hypnogram(minutes, events, s.startTime)
                Text(
                    "Estimation à partir des mouvements et des sons, pas un examen médical.",
                    style = MaterialTheme.typography.labelSmall, color = TextSoft,
                )
            }
        }
        // --- Sons ---
        item {
            Row(Modifier.padding(top = 14.dp, bottom = 6.dp), verticalAlignment = Alignment.CenterVertically) {
                Text("🚨", fontSize = 18.sp)
                Text("  Sons relevés", style = MaterialTheme.typography.titleMedium, modifier = Modifier.weight(1f))
                Text("${events.size}", color = TextSoft)
            }
            Row(Modifier.horizontalScroll(rememberScrollState()), horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                ToggleChip("Tous", filter == null) { filter = null }
                events.map { SoundType.fromName(it.type) }.distinct().forEach { t ->
                    val n = events.count { it.type == t.name }
                    ToggleChip("${t.emoji} ${t.label} $n", filter == t) { filter = if (filter == t) null else t }
                }
            }
        }
        val shown = events.filter { filter == null || it.type == filter!!.name }
        if (shown.isEmpty()) item {
            GlassCard { Text("👮 Rien à signaler, nuit calme !", color = TextSoft) }
        }
        items(shown, key = { it.id }) { e -> EventCard(e, vm, onShare = { p -> vm.shareAudio(ctx, p) }) }

        // --- Au réveil ---
        item {
            GlassCard(title = "Au réveil", icon = "☕") {
                Text("Comment vous sentez-vous ?", color = TextSoft)
                Row(Modifier.fillMaxWidth().padding(vertical = 8.dp), horizontalArrangement = Arrangement.SpaceBetween) {
                    MOODS.forEachIndexed { i, m ->
                        val sel = s.morningMood == i + 1
                        Box(
                            Modifier.size(52.dp).clip(CircleShape)
                                .background(if (sel) PoliceBlue.copy(alpha = 0.35f) else Color(0x10FFFFFF))
                                .border(if (sel) 2.dp else 1.dp, if (sel) PoliceBlue else CardStroke, CircleShape)
                                .clickable { vm.saveSession(s.copy(morningMood = i + 1)) },
                            contentAlignment = Alignment.Center,
                        ) { Text(m, fontSize = 26.sp) }
                    }
                }
                val tags = s.tags.split(",").filter { it.isNotBlank() }.toSet()
                Text("Rapport du soir", color = TextSoft, modifier = Modifier.padding(top = 4.dp))
                FlowRow(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                    JournalTags.ALL.forEach { t ->
                        ToggleChip(t, t in tags) {
                            vm.saveSession(s.copy(tags = (if (t in tags) tags - t else tags + t).joinToString(",")))
                        }
                    }
                }
                OutlinedTextField(
                    notes, { notes = it }, Modifier.fillMaxWidth().padding(top = 8.dp),
                    label = { Text("Notes, rêves…") }, shape = RoundedCornerShape(16.dp),
                )
            }
        }
        val tips = Insights.advice(s, settings.sleepGoalMin)
        if (tips.isNotEmpty()) item {
            GlassCard(title = "Conseils de l'agent", icon = "👮") {
                tips.forEach { Text("• $it", Modifier.padding(vertical = 3.dp)) }
            }
        }
        if (minutes.size > 1) item {
            ExpandableCard("Bruit dans la chambre", "🔊", s.avgNoiseFloorDb?.let { "Bruit de fond moyen %.0f dBFS".format(it) }) {
                NoiseCurve(minutes)
            }
        }
        item { Spacer(Modifier.height(16.dp)) }
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
private fun EventCard(e: SoundEventEntity, vm: SomniaViewModel, onShare: (String) -> Unit) {
    val t = SoundType.fromName(e.type)
    GlassCard {
        Row(verticalAlignment = Alignment.CenterVertically) {
            Box(
                Modifier.size(46.dp).clip(CircleShape).background(t.color().copy(alpha = 0.2f)),
                contentAlignment = Alignment.Center,
            ) { Text(t.emoji, fontSize = 22.sp) }
            Column(Modifier.padding(start = 12.dp).weight(1f)) {
                Text(t.label, color = t.color(), style = MaterialTheme.typography.titleMedium)
                Text(
                    "${Fmt.time(e.timestamp)} · ${Fmt.seconds(e.durationMs)}" +
                        if (t != SoundType.NOISE) " · sûr à ${(e.confidence * 100).toInt()} %" else "",
                    style = MaterialTheme.typography.bodySmall, color = TextSoft,
                )
            }
            IconButton(onClick = { vm.toggleFavorite(e) }) {
                Icon(
                    if (e.favorite) Icons.Filled.Favorite else Icons.Filled.FavoriteBorder, "Garder",
                    tint = if (e.favorite) Color(0xFFFF6B8B) else TextSoft,
                )
            }
        }
        val playing = AudioPlayer.playingPath
        Row(Modifier.padding(top = 10.dp), horizontalArrangement = Arrangement.spacedBy(8.dp), verticalAlignment = Alignment.CenterVertically) {
            e.audioPath?.let { p -> PlayButton(if (playing == p) "⏹ Stop" else "▶ Écouter", playing == p, t.color()) { AudioPlayer.toggle(p) } }
            e.enhancedPath?.let { p -> PlayButton(if (playing == p) "⏹ Stop" else "🗣️ Voix isolée", playing == p, t.color()) { AudioPlayer.toggle(p) } }
            if (e.audioPath == null) Text("Audio non conservé", style = MaterialTheme.typography.labelSmall, color = TextSoft)
            Spacer(Modifier.weight(1f))
            (e.enhancedPath ?: e.audioPath)?.let { p -> IconButton(onClick = { onShare(p) }) { Icon(Icons.Filled.Share, "Partager", tint = TextSoft) } }
            IconButton(onClick = { vm.deleteEvent(e) }) { Icon(Icons.Filled.Delete, "Supprimer", tint = TextSoft) }
        }
    }
}

@Composable
private fun PlayButton(text: String, active: Boolean, color: Color, onClick: () -> Unit) {
    Text(
        text,
        Modifier.clip(RoundedCornerShape(50))
            .background(if (active) color else color.copy(alpha = 0.18f))
            .clickable(onClick = onClick)
            .padding(horizontal = 14.dp, vertical = 9.dp),
        color = if (active) Color(0xFF0B1026) else color,
        fontWeight = FontWeight.SemiBold, fontSize = 14.sp,
    )
}
