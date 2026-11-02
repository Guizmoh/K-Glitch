package com.somnia.app.ui.screens

import android.app.TimePickerDialog
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Checkbox
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.Slider
import androidx.compose.material3.Switch
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp
import com.somnia.app.audio.SoundType
import com.somnia.app.service.TrackingState
import com.somnia.app.ui.Fmt
import com.somnia.app.ui.SomniaViewModel
import com.somnia.app.ui.components.SectionCard
import kotlin.math.roundToInt

@Composable
fun SettingsScreen(vm: SomniaViewModel) {
    val s by vm.settings.collectAsState()
    val live by TrackingState.state.collectAsState()
    val ctx = LocalContext.current

    Column(Modifier.verticalScroll(rememberScrollState()).padding(16.dp)) {
        Text("Réglages", style = MaterialTheme.typography.headlineSmall)
        if (live != null) Text("Les changements s'appliqueront à la prochaine nuit.", color = MaterialTheme.colorScheme.secondary)

        SectionCard("Sensibilité du micro") {
            Text("Écart nécessaire avec le bruit de fond pour déclencher un enregistrement.", style = MaterialTheme.typography.bodySmall)
            LevelSlider(s.micSensitivity, "Peu sensible", "Très sensible") { v -> vm.updateSettings { it.copy(micSensitivity = v) } }
            Text("Niveau ${s.micSensitivity}/10 : son ≥ fond + ${"%.0f".format(23f - 1.9f * s.micSensitivity)} dB", style = MaterialTheme.typography.labelSmall)
        }

        SectionCard("Intensité de captation par type de son") {
            Text(
                "Plus le niveau est haut, plus l'application retient des sons même peu typiques (risque de fausses détections). Plus il est bas, seuls les sons très nets sont gardés.",
                style = MaterialTheme.typography.bodySmall,
            )
            SoundType.entries.filter { it != SoundType.NOISE }.forEach { t ->
                Text("${t.emoji} ${t.label} : ${s.typeSensitivity[t] ?: 5}/10", Modifier.padding(top = 8.dp))
                LevelSlider(s.typeSensitivity[t] ?: 5) { v ->
                    vm.updateSettings { it.copy(typeSensitivity = it.typeSensitivity + (t to v)) }
                }
            }
        }

        SectionCard("Sons à enregistrer") {
            Text("Les sons décochés sont comptés mais l'audio n'est pas conservé.", style = MaterialTheme.typography.bodySmall)
            SoundType.entries.forEach { t ->
                Row(verticalAlignment = Alignment.CenterVertically) {
                    Checkbox(t in s.recordTypes, { on ->
                        vm.updateSettings { it.copy(recordTypes = if (on) it.recordTypes + t else it.recordTypes - t) }
                    })
                    Text("${t.emoji} ${t.label}" + if (t == SoundType.NOISE) " (bruits non identifiés)" else "")
                }
            }
            SwitchRow("Créer une version « voix isolée » (réduction du bruit ambiant)", s.enhanceVoice) { v ->
                vm.updateSettings { it.copy(enhanceVoice = v) }
            }
        }

        SectionCard("Nuit") {
            SwitchRow("Mesurer les mouvements (téléphone sur le matelas)", s.trackMovement) { v -> vm.updateSettings { it.copy(trackMovement = v) } }
            Text("Délai avant détection : ${s.startDelayMin} min", Modifier.padding(top = 8.dp))
            Slider(s.startDelayMin.toFloat(), { v -> vm.updateSettings { it.copy(startDelayMin = (v / 5).roundToInt() * 5) } }, valueRange = 0f..60f, steps = 11)
            Text("Objectif de sommeil : ${Fmt.duration(s.sleepGoalMin)}")
            Slider(s.sleepGoalMin.toFloat(), { v -> vm.updateSettings { it.copy(sleepGoalMin = (v / 30).roundToInt() * 30) } }, valueRange = 300f..660f, steps = 11)
            Text("Garder les enregistrements : " + if (s.retentionDays == 0) "toujours" else "${s.retentionDays} jours (sauf favoris ❤)")
            Slider(s.retentionDays.toFloat(), { v -> vm.updateSettings { it.copy(retentionDays = (v / 7).roundToInt() * 7) } }, valueRange = 0f..91f, steps = 12)
        }

        SectionCard("Réveil intelligent") {
            SwitchRow("Activer", s.smartAlarmEnabled) { v -> vm.updateSettings { it.copy(smartAlarmEnabled = v) } }
            Text(
                "Sonne pendant un moment de sommeil léger (mouvement détecté) dans la fenêtre choisie, sinon à l'heure exacte.",
                style = MaterialTheme.typography.bodySmall,
            )
            OutlinedButton(onClick = {
                TimePickerDialog(ctx, { _, h, m -> vm.updateSettings { it.copy(alarmHour = h, alarmMinute = m) } }, s.alarmHour, s.alarmMinute, true).show()
            }) { Text("Au plus tard à %02d:%02d".format(s.alarmHour, s.alarmMinute)) }
            Text("Fenêtre : ${s.alarmWindowMin} min avant")
            Slider(s.alarmWindowMin.toFloat(), { v -> vm.updateSettings { it.copy(alarmWindowMin = (v / 5).roundToInt() * 5) } }, valueRange = 0f..45f, steps = 8)
        }

        SectionCard("Rappel du coucher") {
            SwitchRow("Me rappeler d'aller dormir", s.bedtimeReminderEnabled) { v -> vm.updateSettings { it.copy(bedtimeReminderEnabled = v) } }
            OutlinedButton(onClick = {
                TimePickerDialog(ctx, { _, h, m -> vm.updateSettings { it.copy(bedtimeHour = h, bedtimeMinute = m) } }, s.bedtimeHour, s.bedtimeMinute, true).show()
            }) { Text("À %02d:%02d".format(s.bedtimeHour, s.bedtimeMinute)) }
        }

        SectionCard("Données") {
            Text("Tout reste sur le téléphone : aucun son n'est envoyé sur Internet.", style = MaterialTheme.typography.bodySmall)
            OutlinedButton(onClick = { vm.exportCsv(ctx) }, Modifier.fillMaxWidth()) { Text("Exporter mes nuits (CSV)") }
        }
    }
}

@Composable
private fun LevelSlider(value: Int, left: String? = null, right: String? = null, onChange: (Int) -> Unit) {
    Slider(value.toFloat(), { onChange(it.roundToInt()) }, valueRange = 1f..10f, steps = 8)
    if (left != null && right != null) Row(Modifier.fillMaxWidth()) {
        Text(left, Modifier.weight(1f), style = MaterialTheme.typography.labelSmall)
        Text(right, style = MaterialTheme.typography.labelSmall)
    }
}

@Composable
private fun SwitchRow(label: String, checked: Boolean, onChange: (Boolean) -> Unit) {
    Row(verticalAlignment = Alignment.CenterVertically, modifier = Modifier.fillMaxWidth()) {
        Text(label, Modifier.weight(1f))
        Switch(checked, onChange)
    }
}
