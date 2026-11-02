package com.somnia.app.ui.screens

import android.app.TimePickerDialog
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
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
import androidx.compose.runtime.setValue
import androidx.compose.runtime.remember
import androidx.compose.foundation.clickable
import androidx.compose.ui.draw.clip
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp
import com.somnia.app.audio.SoundType
import com.somnia.app.data.CapturePreset
import com.somnia.app.service.TrackingState
import com.somnia.app.ui.Fmt
import com.somnia.app.ui.SomniaViewModel
import com.somnia.app.ui.components.ExpandableCard
import com.somnia.app.ui.components.GlassCard
import com.somnia.app.ui.components.ScreenHeader
import com.somnia.app.ui.components.Segmented
import com.somnia.app.ui.theme.Badge
import com.somnia.app.ui.theme.TextSoft
import kotlin.math.roundToInt

@Composable
fun SettingsScreen(vm: SomniaViewModel, onTutorial: () -> Unit = {}) {
    val s by vm.settings.collectAsState()
    val live by TrackingState.state.collectAsState()
    val ctx = LocalContext.current
    val preset = CapturePreset.of(s)

    Column(Modifier.verticalScroll(rememberScrollState()).padding(horizontal = 20.dp)) {
        ScreenHeader("Réglages", "Simple par défaut, précis si besoin")
        if (live != null) Text("Les changements s'appliqueront à la prochaine nuit.", color = Badge, modifier = Modifier.padding(bottom = 6.dp))

        GlassCard(title = "Que doit-on capter ?", icon = "🎙️") {
            Segmented(CapturePreset.entries.map { it.label }, preset?.ordinal ?: -1) { i ->
                vm.updateSettings { CapturePreset.entries[i].apply(it) }
            }
            Spacer(Modifier.height(10.dp))
            Text(preset?.description ?: "Réglage personnalisé (voir « Réglage fin » ci-dessous).", color = TextSoft, style = MaterialTheme.typography.bodySmall)
            var testing2 by remember { androidx.compose.runtime.mutableStateOf(false) }
            OutlinedButton(onClick = { testing2 = true }, enabled = live == null, modifier = Modifier.fillMaxWidth().padding(top = 8.dp)) {
                Text("Tester : qu'est-ce qui déclenche ?")
            }
            if (testing2) MicTestDialog(
                s, com.somnia.app.audio.MicChoice.label(ctx, s.micSource),
                onSensitivity = { v -> vm.updateSettings { it.copy(micSensitivity = v) } },
            ) { testing2 = false }
        }

        GlassCard(title = "Micro utilisé", icon = "🎙️") {
            val options = remember { com.somnia.app.audio.MicChoice.BUILT_IN + com.somnia.app.audio.MicChoice.externalOptions(ctx) }
            options.forEach { o ->
                Row(
                    Modifier.fillMaxWidth().clip(androidx.compose.foundation.shape.RoundedCornerShape(12.dp))
                        .clickable { vm.updateSettings { it.copy(micSource = o.key) } }.padding(vertical = 4.dp),
                    verticalAlignment = Alignment.CenterVertically,
                ) {
                    androidx.compose.material3.RadioButton(s.micSource == o.key, { vm.updateSettings { it.copy(micSource = o.key) } })
                    Column {
                        Text(o.label, style = MaterialTheme.typography.titleSmall)
                        Text(o.detail, style = MaterialTheme.typography.bodySmall, color = TextSoft)
                    }
                }
            }
            var testing by remember { androidx.compose.runtime.mutableStateOf(false) }
            OutlinedButton(
                onClick = { testing = true }, enabled = live == null,
                modifier = Modifier.fillMaxWidth().padding(top = 6.dp),
            ) { Text(if (live == null) "Tester le micro et la détection" else "Test impossible pendant la nuit") }
            if (testing) MicTestDialog(
                s, com.somnia.app.audio.MicChoice.label(ctx, s.micSource),
                onSensitivity = { v -> vm.updateSettings { it.copy(micSensitivity = v) } },
            ) { testing = false }
            Text(
                "Branchez un micro externe (casque, USB) puis revenez ici pour le voir apparaître. Selon le téléphone, certains choix utilisent le micro le plus proche disponible.",
                style = MaterialTheme.typography.bodySmall, color = TextSoft, modifier = Modifier.padding(top = 6.dp),
            )
        }

        ExpandableCard("Réglage fin par type de son", "🎛️", "Sensibilité du micro et intensité pour rires, pleurs, cris…") {
            Text("Micro : ${s.micSensitivity}/10", style = MaterialTheme.typography.titleSmall)
            Text("Écart nécessaire avec le bruit de fond : +${"%.0f".format(23f - 1.9f * s.micSensitivity)} dB", style = MaterialTheme.typography.bodySmall, color = TextSoft)
            LevelSlider(s.micSensitivity) { v -> vm.updateSettings { it.copy(micSensitivity = v) } }
            Text(
                "Pour chaque type : plus c'est haut, plus les sons discrets ou peu typiques sont retenus.",
                style = MaterialTheme.typography.bodySmall, color = TextSoft, modifier = Modifier.padding(top = 8.dp),
            )
            SoundType.entries.filter { it != SoundType.NOISE }.forEach { t ->
                Text("${t.label} : ${s.typeSensitivity[t] ?: 5}/10", Modifier.padding(top = 10.dp))
                LevelSlider(s.typeSensitivity[t] ?: 5) { v -> vm.updateSettings { it.copy(typeSensitivity = it.typeSensitivity + (t to v)) } }
            }
        }

        ExpandableCard("Sons à garder en audio", "💾", "Les autres sont seulement comptés") {
            SoundType.entries.forEach { t ->
                Row(verticalAlignment = Alignment.CenterVertically) {
                    Checkbox(t in s.recordTypes, { on ->
                        vm.updateSettings { it.copy(recordTypes = if (on) it.recordTypes + t else it.recordTypes - t) }
                    })
                    Text("${t.label}" + if (t == SoundType.NOISE) " (non identifiés)" else "")
                }
            }
            SwitchRow("Version « voix isolée » du bruit ambiant", s.enhanceVoice) { v -> vm.updateSettings { it.copy(enhanceVoice = v) } }
            Text("Garder les fichiers : " + if (s.retentionDays == 0) "toujours" else "${s.retentionDays} jours (sauf favoris )", Modifier.padding(top = 8.dp))
            Slider(s.retentionDays.toFloat(), { v -> vm.updateSettings { it.copy(retentionDays = (v / 7).roundToInt() * 7) } }, valueRange = 0f..91f, steps = 12)
        }

        GlassCard(title = "Réveil intelligent", icon = "⏰") {
            SwitchRow("Me réveiller en sommeil léger", s.smartAlarmEnabled) { v -> vm.updateSettings { it.copy(smartAlarmEnabled = v) } }
            if (s.smartAlarmEnabled) {
                OutlinedButton(onClick = {
                    TimePickerDialog(ctx, { _, h, m -> vm.updateSettings { it.copy(alarmHour = h, alarmMinute = m) } }, s.alarmHour, s.alarmMinute, true).show()
                }, Modifier.fillMaxWidth()) { Text("Au plus tard à %02d:%02d".format(s.alarmHour, s.alarmMinute)) }
                Text("Peut sonner jusqu'à ${s.alarmWindowMin} min plus tôt", Modifier.padding(top = 8.dp), color = TextSoft)
                Slider(s.alarmWindowMin.toFloat(), { v -> vm.updateSettings { it.copy(alarmWindowMin = (v / 5).roundToInt() * 5) } }, valueRange = 0f..45f, steps = 8)
            }
        }

        GlassCard(title = "Rappel du coucher", icon = "🔔") {
            SwitchRow("Me rappeler d'aller dormir", s.bedtimeReminderEnabled) { v -> vm.updateSettings { it.copy(bedtimeReminderEnabled = v) } }
            if (s.bedtimeReminderEnabled) {
                OutlinedButton(onClick = {
                    TimePickerDialog(ctx, { _, h, m -> vm.updateSettings { it.copy(bedtimeHour = h, bedtimeMinute = m) } }, s.bedtimeHour, s.bedtimeMinute, true).show()
                }, Modifier.fillMaxWidth()) { Text("À %02d:%02d".format(s.bedtimeHour, s.bedtimeMinute)) }
            }
        }

        ExpandableCard("Ma nuit", "🛏️", "Objectif ${Fmt.duration(s.sleepGoalMin)} · mouvements · délai") {
            Text("Objectif de sommeil : ${Fmt.duration(s.sleepGoalMin)}")
            Slider(s.sleepGoalMin.toFloat(), { v -> vm.updateSettings { it.copy(sleepGoalMin = (v / 30).roundToInt() * 30) } }, valueRange = 300f..660f, steps = 11)
            SwitchRow("Mesurer les mouvements (téléphone sur le matelas)", s.trackMovement) { v -> vm.updateSettings { it.copy(trackMovement = v) } }
            Text("Ignorer les ${s.startDelayMin} premières minutes", Modifier.padding(top = 8.dp))
            Slider(s.startDelayMin.toFloat(), { v -> vm.updateSettings { it.copy(startDelayMin = (v / 5).roundToInt() * 5) } }, valueRange = 0f..60f, steps = 11)
        }

        GlassCard(title = "Aide", icon = "👮", onClick = onTutorial) {
            Text("Revoir le tuto de l'agent du Dodo", color = TextSoft)
            Text("Astuce : touchez la tête du policier pour changer de personnage (aussi sur l'icône du téléphone).", color = TextSoft, style = MaterialTheme.typography.bodySmall, modifier = Modifier.padding(top = 4.dp))
        }

        GlassCard(title = "Mes données", icon = "🔒") {
            Text("Tout reste sur le téléphone : aucun son n'est envoyé sur Internet.", color = TextSoft, style = MaterialTheme.typography.bodySmall)
            OutlinedButton(onClick = { vm.exportCsv(ctx) }, Modifier.fillMaxWidth().padding(top = 8.dp)) { Text("Exporter mes nuits (CSV)") }
        }
        Spacer(Modifier.height(16.dp))
    }
}

@Composable
private fun LevelSlider(value: Int, onChange: (Int) -> Unit) {
    Slider(value.toFloat(), { onChange(it.roundToInt()) }, valueRange = 1f..10f, steps = 8)
}

@Composable
private fun SwitchRow(label: String, checked: Boolean, onChange: (Boolean) -> Unit) {
    Row(verticalAlignment = Alignment.CenterVertically, modifier = Modifier.fillMaxWidth().padding(vertical = 4.dp)) {
        Text(label, Modifier.weight(1f))
        Switch(checked, onChange)
    }
}
