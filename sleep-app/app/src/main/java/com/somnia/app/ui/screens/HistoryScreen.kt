package com.somnia.app.ui.screens

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.setValue
import androidx.compose.material.icons.rounded.Star
import androidx.compose.material.icons.rounded.StarBorder
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.somnia.app.audio.SoundType
import com.somnia.app.data.SleepSession
import com.somnia.app.ui.Fmt
import com.somnia.app.ui.SomniaViewModel
import com.somnia.app.ui.components.GlassCard
import com.somnia.app.ui.components.Pill
import com.somnia.app.ui.components.PoliceLogo
import com.somnia.app.ui.components.ScoreRing
import com.somnia.app.ui.components.ScreenHeader
import com.somnia.app.ui.theme.AwakeColor
import com.somnia.app.ui.theme.Badge
import com.somnia.app.ui.theme.TextSoft
import com.somnia.app.ui.theme.color
import com.somnia.app.ui.theme.scoreColor

@Composable
fun HistoryScreen(vm: SomniaViewModel, open: (Long) -> Unit) {
    val sessions by vm.sessions.collectAsState()
    HistoryContent(sessions, open, onStar = { vm.toggleNightFavorite(it) })
}

@Composable
fun HistoryContent(all: List<SleepSession>, open: (Long) -> Unit, onStar: (SleepSession) -> Unit = {}) {
    var onlyFav by androidx.compose.runtime.saveable.rememberSaveable { androidx.compose.runtime.mutableStateOf(false) }
    val sessions = if (onlyFav) all.filter { it.favorite } else all
    LazyColumn(Modifier.padding(horizontal = 20.dp)) {
        item {
            ScreenHeader("Mes nuits", "${all.size} nuit(s) dans le registre · ${all.count { it.favorite }} favorite(s)")
            com.somnia.app.ui.components.Segmented(listOf("Toutes", "Favorites"), if (onlyFav) 1 else 0) { onlyFav = it == 1 }
            Spacer(Modifier.height(6.dp))
        }
        if (onlyFav && sessions.isEmpty()) item {
            Text("Aucune nuit favorite. Touchez l'étoile d'une nuit pour la garder.", color = TextSoft, modifier = Modifier.padding(vertical = 16.dp))
        }
        if (sessions.isEmpty()) item {
            Column(Modifier.padding(top = 60.dp), horizontalAlignment = Alignment.CenterHorizontally) {
                PoliceLogo(96.dp, awake = false)
                Spacer(Modifier.height(16.dp))
                Text(
                    "Aucune nuit pour l'instant.\nLancez votre première surveillance depuis « Ce soir ».",
                    textAlign = TextAlign.Center, color = TextSoft,
                )
            }
        }
        items(sessions, key = { it.id }) { s ->
            GlassCard(onClick = { open(s.id) }) {
                Row(verticalAlignment = Alignment.CenterVertically) {
                    Box(contentAlignment = Alignment.Center) {
                        ScoreRing(s.score, Modifier.size(58.dp))
                        Text(s.score?.toString() ?: "–", style = MaterialTheme.typography.titleMedium, color = scoreColor(s.score))
                    }
                    Column(Modifier.padding(start = 16.dp).weight(1f)) {
                        Text(Fmt.day(s.startTime), style = MaterialTheme.typography.titleMedium)
                        Text(
                            if (s.endTime == null) "En cours…"
                            else "${Fmt.duration(s.deepMinutes + s.lightMinutes)} de sommeil · ${Fmt.time(s.startTime)} → ${Fmt.time(s.endTime)}",
                            style = MaterialTheme.typography.bodySmall, color = TextSoft,
                        )
                        Row(Modifier.padding(top = 8.dp), horizontalArrangement = Arrangement.spacedBy(6.dp)) {
                            if (s.snoringMinutes > 0) Pill("😴 ${s.snoringMinutes} min", SoundType.SNORING.color())
                            if (s.awakenings > 0) Pill("👀 ${s.awakenings} réveil(s)", AwakeColor)
                        }
                    }
                    androidx.compose.material3.IconButton(onClick = { onStar(s) }) {
                        androidx.compose.material3.Icon(
                            if (s.favorite) androidx.compose.material.icons.Icons.Rounded.Star else androidx.compose.material.icons.Icons.Rounded.StarBorder,
                            if (s.favorite) "Retirer des favoris" else "Mettre en favori",
                            tint = if (s.favorite) Badge else TextSoft,
                        )
                    }
                }
            }
        }
        item { Spacer(Modifier.height(12.dp)) }
    }
}
