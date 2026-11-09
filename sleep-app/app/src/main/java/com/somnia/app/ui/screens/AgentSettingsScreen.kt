package com.somnia.app.ui.screens

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.itemsIndexed
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.filled.ArrowBack
import androidx.compose.material.icons.filled.Delete
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Switch
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import com.somnia.app.data.AgentLine
import com.somnia.app.ui.AgentLines
import com.somnia.app.ui.SomniaViewModel
import com.somnia.app.ui.components.GlassCard
import com.somnia.app.ui.components.PoliceLogo
import com.somnia.app.ui.components.SpeechBubble
import com.somnia.app.ui.theme.TextSoft
import java.util.Calendar

/** Page « Le policier » : humeurs selon l'heure et phrases qu'il dit (texte, heures, activé ou non). */
@Composable
fun AgentSettingsScreen(vm: SomniaViewModel, back: () -> Unit) {
    val s by vm.settings.collectAsState()
    val lines = s.agentLines
    var confirmReset by remember { mutableStateOf(false) }
    fun setLines(new: List<AgentLine>) = vm.updateSettings { it.copy(agentLines = new) }
    fun update(i: Int, f: (AgentLine) -> AgentLine) = setLines(lines.toMutableList().also { it[i] = f(it[i]) })

    LazyColumn(Modifier.padding(horizontal = 20.dp)) {
        item {
            Row(Modifier.padding(top = 8.dp), verticalAlignment = Alignment.CenterVertically) {
                IconButton(onClick = back) { Icon(Icons.AutoMirrored.Filled.ArrowBack, "Retour") }
                Text("Le policier", style = MaterialTheme.typography.headlineSmall)
            }
        }
        // Aperçu : ce qu'il dit maintenant
        item {
            val hour = Calendar.getInstance().get(Calendar.HOUR_OF_DAY)
            GlassCard {
                Column(Modifier.fillMaxWidth(), horizontalAlignment = Alignment.CenterHorizontally) {
                    PoliceLogo(96.dp, awake = true)
                    Spacer(Modifier.height(4.dp))
                    val now = AgentLines.forHour(hour, lines)
                    if (now != null) SpeechBubble(now)
                    else Text("À $hour h, il ne dit rien.", color = TextSoft, style = MaterialTheme.typography.bodySmall)
                }
            }
        }
        item {
            GlassCard(title = "Humeurs", icon = "👮") {
                val m = s.moods
                SwitchRow("Humeurs selon l'heure", m.enabled) { v -> vm.updateSettings { it.copy(moods = it.moods.copy(enabled = v)) } }
                if (m.enabled) {
                    HourRangeRow("En colère", m.angryStart, m.angryEnd) { a, b -> vm.updateSettings { it.copy(moods = it.moods.copy(angryStart = a, angryEnd = b)) } }
                    HourRangeRow("Il rigole", m.joyStart, m.joyEnd) { a, b -> vm.updateSettings { it.copy(moods = it.moods.copy(joyStart = a, joyEnd = b)) } }
                    Text("Après le rire : il sourit jusqu'à minuit, puis il est neutre jusqu'à la colère.", style = MaterialTheme.typography.bodySmall, color = TextSoft)
                } else {
                    Text("Désactivé : grand sourire toute la journée, sourire à partir de minuit.", style = MaterialTheme.typography.bodySmall, color = TextSoft)
                }
            }
        }
        item {
            Text("Ce qu'il dit", style = MaterialTheme.typography.titleMedium, modifier = Modifier.padding(top = 12.dp))
            Text(
                "Chaque phrase s'affiche dans sa bulle pendant ses heures. Désactivez une phrase pour qu'il ne dise rien à ces heures-là.",
                style = MaterialTheme.typography.bodySmall, color = TextSoft,
            )
        }
        itemsIndexed(lines) { i, line ->
            GlassCard {
                Row(verticalAlignment = Alignment.CenterVertically) {
                    Column(Modifier.weight(1f)) {
                        HourRangeRow(if (line.enabled) "Il dit" else "Il ne dit rien", line.startHour, line.endHour) { a, b ->
                            update(i) { it.copy(startHour = a, endHour = b) }
                        }
                    }
                    Switch(line.enabled, { v -> update(i) { it.copy(enabled = v) } })
                }
                if (line.enabled) OutlinedTextField(
                    line.text, { t -> update(i) { it.copy(text = t) } }, Modifier.fillMaxWidth(),
                    shape = RoundedCornerShape(14.dp), label = { Text("Phrase") },
                )
                Row(Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.End) {
                    IconButton(onClick = { setLines(lines.filterIndexed { j, _ -> j != i }) }) { Icon(Icons.Filled.Delete, "Supprimer cette phrase", tint = TextSoft) }
                }
            }
        }
        item {
            OutlinedButton(onClick = { setLines(lines + AgentLine(12, 13, "")) }, modifier = Modifier.fillMaxWidth().padding(top = 8.dp)) {
                Text("Ajouter une phrase")
            }
            TextButton(onClick = { confirmReset = true }, modifier = Modifier.fillMaxWidth()) { Text("Revenir aux phrases d'origine") }
            Text(
                "Si deux phrases couvrent la même heure, c'est la première de la liste qui est dite.",
                style = MaterialTheme.typography.labelSmall, color = TextSoft, modifier = Modifier.padding(bottom = 24.dp),
            )
        }
    }
    if (confirmReset) AlertDialog(
        onDismissRequest = { confirmReset = false },
        title = { Text("Revenir aux phrases d'origine ?") },
        text = { Text("Vos phrases modifiées seront remplacées.") },
        confirmButton = { TextButton(onClick = { setLines(AgentLines.DEFAULTS); confirmReset = false }) { Text("Rétablir") } },
        dismissButton = { TextButton(onClick = { confirmReset = false }) { Text("Annuler") } },
    )
}
