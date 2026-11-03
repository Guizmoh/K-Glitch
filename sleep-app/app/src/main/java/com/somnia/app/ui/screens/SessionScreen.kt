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
import com.somnia.app.ui.theme.icon
import androidx.compose.material.icons.rounded.SentimentDissatisfied
import androidx.compose.material.icons.rounded.Pause
import androidx.compose.material.icons.rounded.Star
import androidx.compose.material.icons.rounded.StarBorder
import androidx.compose.material.icons.rounded.PlayArrow
import androidx.compose.material.icons.rounded.SentimentNeutral
import androidx.compose.material.icons.rounded.SentimentSatisfied
import androidx.compose.material.icons.rounded.SentimentVeryDissatisfied
import androidx.compose.material.icons.rounded.SentimentVerySatisfied
import com.somnia.app.ui.theme.scoreColor

private val MOODS = listOf(
    Icons.Rounded.SentimentVeryDissatisfied, Icons.Rounded.SentimentDissatisfied, Icons.Rounded.SentimentNeutral,
    Icons.Rounded.SentimentSatisfied, Icons.Rounded.SentimentVerySatisfied,
)

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
                IconButton(onClick = { vm.toggleNightFavorite(s) }) {
                    Icon(
                        if (s.favorite) Icons.Rounded.Star else Icons.Rounded.StarBorder,
                        if (s.favorite) "Retirer des favoris" else "Mettre la nuit en favori",
                        tint = if (s.favorite) com.somnia.app.ui.theme.Badge else TextSoft,
                    )
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
                                s.score >= 80 -> "Excellente nuit"
                                s.score >= 60 -> "Bonne nuit"
                                s.score >= 40 -> "Nuit moyenne"
                                else -> "Nuit difficile"
                            },
                            style = MaterialTheme.typography.titleLarge,
                        )
                        Spacer(Modifier.height(4.dp))
                        Text("${Fmt.time(s.startTime)} → ${s.endTime?.let { Fmt.time(it) } ?: "…"}", color = TextSoft)
                        s.sleepOnset?.let { Text("Endormi·e en ${(it - s.startTime) / 60_000} min", color = TextSoft) }
                        s.wakeTime?.let { Text("Réveil vers ${Fmt.time(it)}", color = TextSoft) }
                    }
                }
            }
        }
        item {
            TileRow {
                StatTile("😴", Fmt.duration(s.deepMinutes + s.lightMinutes), "Sommeil", Modifier.weight(1f))
                StatTile("🌊", Fmt.duration(s.deepMinutes), "Profond", Modifier.weight(1f), accent = com.somnia.app.ui.theme.LightColor)
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
                if (s.calibrationNights > 0) Text(
                    "Calibré sur vos ${s.calibrationNights} nuits précédentes.",
                    style = MaterialTheme.typography.labelSmall, color = TextSoft, modifier = Modifier.padding(top = 4.dp),
                )
                Text(
                    "Estimation à partir des mouvements et des sons, pas un examen médical.",
                    style = MaterialTheme.typography.labelSmall, color = TextSoft,
                )
            }
        }
        // --- Sons ---
        item {
            Row(Modifier.padding(top = 14.dp, bottom = 6.dp), verticalAlignment = Alignment.CenterVertically) {
                Text("Sons relevés", style = MaterialTheme.typography.titleMedium, modifier = Modifier.weight(1f))
                Text("${events.size}", color = TextSoft)
            }
            Row(Modifier.horizontalScroll(rememberScrollState()), horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                ToggleChip("Tous", filter == null) { filter = null }
                events.map { SoundType.fromName(it.type) }.distinct().forEach { t ->
                    val n = events.count { it.type == t.name }
                    ToggleChip("${t.label} · $n", filter == t) { filter = if (filter == t) null else t }
                }
            }
        }
        val shown = events.filter { filter == null || it.type == filter!!.name }
        if (shown.isEmpty()) item {
            GlassCard { Text("Rien à signaler, nuit calme !", color = TextSoft) }
        }
        items(shown, key = { it.id }) { e -> EventCard(e, vm) }

        // --- Au réveil ---
        item {
            GlassCard(title = "Au réveil", icon = "☕") {
                Text("Comment vous sentez-vous ?", color = TextSoft)
                Row(Modifier.fillMaxWidth().padding(vertical = 8.dp), horizontalArrangement = Arrangement.SpaceBetween) {
                    MOODS.forEachIndexed { i, m ->
                        val sel = s.morningMood == i + 1
                        Box(
                            Modifier.size(52.dp).clip(RoundedCornerShape(14.dp))
                                .background(if (sel) PoliceBlue.copy(alpha = 0.14f) else Color.Transparent)
                                .border(1.dp, if (sel) PoliceBlue else CardStroke, RoundedCornerShape(14.dp))
                                .clickable { vm.saveSession(s.copy(morningMood = i + 1)) },
                            contentAlignment = Alignment.Center,
                        ) { Icon(m, null, tint = if (sel) PoliceBlue else TextSoft, modifier = Modifier.size(28.dp)) }
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
private fun EventCard(e: SoundEventEntity, vm: SomniaViewModel) {
    val t = SoundType.fromName(e.type)
    GlassCard {
        Row(verticalAlignment = Alignment.CenterVertically) {
            Box(
                Modifier.size(42.dp).clip(RoundedCornerShape(12.dp)).background(t.color().copy(alpha = 0.14f)),
                contentAlignment = Alignment.Center,
            ) { Icon(t.icon(), null, tint = t.color(), modifier = Modifier.size(22.dp)) }
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
        // Paroles transcrites
        val ctx = LocalContext.current
        val modelReady = remember { com.somnia.app.transcribe.Transcriber.isReady(ctx) }
        var transcribing by remember(e.id) { mutableStateOf(false) }
        if (e.transcript != null) {
            Text(
                "« ${e.transcript} »",
                Modifier.padding(top = 10.dp).fillMaxWidth().clip(RoundedCornerShape(12.dp)).background(Color(0x22FFFFFF)).padding(12.dp),
                style = MaterialTheme.typography.titleMedium.copy(fontFamily = androidx.compose.ui.text.font.FontFamily.Serif),
            )
        } else if ((t == SoundType.SPEECH || t == SoundType.NIGHT_TERROR) && e.audioPath != null) {
            if (modelReady) {
                ToggleChip(if (transcribing) "Transcription…" else "Écrire ce qui est dit", false) {
                    if (!transcribing) { transcribing = true; vm.transcribe(e) }
                }
            } else {
                Text("Activez « Transcription des paroles » dans Réglages pour lire ce qui est dit.", style = MaterialTheme.typography.labelSmall, color = TextSoft, modifier = Modifier.padding(top = 6.dp))
            }
        }

        // Choix de la version écoutée : original ou voix isolée
        var useEnhanced by remember(e.id) { mutableStateOf(false) }
        val path = if (useEnhanced) e.enhancedPath ?: e.audioPath else e.audioPath
        if (path != null) {
            val current = AudioPlayer.playingPath == path
            val progress = if (current && AudioPlayer.durationMs > 0) AudioPlayer.positionMs / AudioPlayer.durationMs.toFloat() else 0f
            com.somnia.app.ui.components.Waveform(
                path, progress, t.color(),
                onSeek = { f ->
                    val total = if (current) AudioPlayer.durationMs else e.durationMs.toInt()
                    AudioPlayer.play(path, (f * total).toInt())
                },
                modifier = Modifier.padding(top = 12.dp),
            )
            Row(Modifier.fillMaxWidth().padding(top = 6.dp), verticalAlignment = Alignment.CenterVertically) {
                Box(
                    Modifier.size(44.dp).clip(CircleShape).background(t.color()).clickable { AudioPlayer.toggle(path) },
                    contentAlignment = Alignment.Center,
                ) {
                    Icon(
                        if (current && AudioPlayer.isPlaying) Icons.Rounded.Pause else Icons.Rounded.PlayArrow,
                        if (current && AudioPlayer.isPlaying) "Pause" else "Écouter", tint = Color(0xFF13305F),
                    )
                }
                Text(
                    "  " + fmtMs(if (current) AudioPlayer.positionMs else 0) + " / " + fmtMs(if (current && AudioPlayer.durationMs > 0) AudioPlayer.durationMs else e.durationMs.toInt()),
                    style = MaterialTheme.typography.labelLarge, color = TextSoft,
                )
            }
            if (e.enhancedPath != null) {
                Spacer(Modifier.height(10.dp))
                com.somnia.app.ui.components.Segmented(listOf("Original", "Voix claire"), if (useEnhanced) 1 else 0) {
                    AudioPlayer.stop(); useEnhanced = it == 1
                }
                Text(
                    if (useEnhanced) "Voix claire : le bruit de fond (ventilateur, rue…) est retiré et le volume remonté."
                    else "Original : le son tel qu'il a été enregistré.",
                    style = MaterialTheme.typography.labelSmall, color = TextSoft, modifier = Modifier.padding(top = 6.dp),
                )
            }
        } else {
            Text("Audio non conservé", style = MaterialTheme.typography.labelSmall, color = TextSoft, modifier = Modifier.padding(top = 8.dp))
        }
        var correcting by remember(e.id) { mutableStateOf(false) }
        if (correcting) CorrectDialog(t, onPick = { right ->
            correcting = false
            vm.correctEvent(e, right)?.let { android.widget.Toast.makeText(ctx, it, android.widget.Toast.LENGTH_LONG).show() }
        }, onClose = { correcting = false })
        Row(Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.End, verticalAlignment = Alignment.CenterVertically) {
            if (e.userCorrected) Text("corrigé ", style = MaterialTheme.typography.labelSmall, color = TextSoft)
            TextButton(onClick = { correcting = true }) { Text("Corriger") }
            if (path != null) TextButton(onClick = { vm.shareAudio(ctx, e, path, useEnhanced && e.enhancedPath != null) }) {
                Icon(Icons.Filled.Share, null, Modifier.size(16.dp))
                Text(" Partager")
            }
            IconButton(onClick = { vm.deleteEvent(e) }) { Icon(Icons.Filled.Delete, "Supprimer", tint = TextSoft) }
        }
    }
}

/** « Ce n'était pas ça » : choisir le bon type de son. */
@Composable
private fun CorrectDialog(current: SoundType, onPick: (SoundType) -> Unit, onClose: () -> Unit) {
    AlertDialog(
        onDismissRequest = onClose,
        title = { Text("C'était quoi, en vrai ?") },
        text = {
            Column {
                Text("L'appli s'ajuste à chaque correction.", style = MaterialTheme.typography.bodySmall, color = TextSoft)
                SoundType.entries.filter { it != current }.forEach { ty ->
                    Row(
                        Modifier.fillMaxWidth().clip(RoundedCornerShape(10.dp)).clickable { onPick(ty) }.padding(vertical = 10.dp, horizontal = 4.dp),
                        verticalAlignment = Alignment.CenterVertically,
                    ) {
                        Icon(ty.icon(), null, tint = ty.color(), modifier = Modifier.size(22.dp))
                        Text("  " + if (ty == SoundType.NOISE) "Rien d'important (drap, bruit…)" else ty.label)
                    }
                }
            }
        },
        confirmButton = { TextButton(onClick = onClose) { Text("Annuler") } },
    )
}

private fun fmtMs(ms: Int): String = "%d:%02d".format(ms / 60_000, (ms / 1000) % 60)

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
