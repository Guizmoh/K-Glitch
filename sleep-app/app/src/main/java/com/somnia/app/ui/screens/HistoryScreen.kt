package com.somnia.app.ui.screens

import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import com.somnia.app.ui.Fmt
import com.somnia.app.ui.SomniaViewModel
import com.somnia.app.ui.components.ScoreRing
import com.somnia.app.ui.components.SectionCard

@Composable
fun HistoryScreen(vm: SomniaViewModel, open: (Long) -> Unit) {
    val sessions by vm.sessions.collectAsState()
    LazyColumn(Modifier.padding(horizontal = 16.dp)) {
        item { Text("Mes nuits", style = MaterialTheme.typography.headlineSmall, modifier = Modifier.padding(vertical = 12.dp)) }
        if (sessions.isEmpty()) item { Text("Aucune nuit enregistrée pour l'instant.") }
        items(sessions, key = { it.id }) { s ->
            SectionCard(modifier = Modifier.clickable { open(s.id) }) {
                Row(verticalAlignment = Alignment.CenterVertically) {
                    Box(contentAlignment = Alignment.Center) {
                        ScoreRing(s.score, Modifier.size(56.dp))
                        Text(s.score?.toString() ?: "–")
                    }
                    Column(Modifier.padding(start = 16.dp)) {
                        Text(Fmt.day(s.startTime), style = MaterialTheme.typography.titleSmall)
                        Text(
                            if (s.endTime == null) "En cours…"
                            else "${Fmt.time(s.startTime)} → ${Fmt.time(s.endTime)} · sommeil ${Fmt.duration(s.deepMinutes + s.lightMinutes)}",
                            style = MaterialTheme.typography.bodySmall,
                        )
                        if (s.snoringMinutes > 0 || s.awakenings > 0) {
                            Text("😴 ${s.snoringMinutes} min de ronflement · ${s.awakenings} réveil(s)", style = MaterialTheme.typography.bodySmall)
                        }
                        if (s.morningMood > 0) Text("Humeur : " + "⭐".repeat(s.morningMood), style = MaterialTheme.typography.bodySmall)
                    }
                }
            }
        }
    }
}
