package com.somnia.app.ui.screens

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.ExperimentalLayoutApi
import androidx.compose.foundation.layout.FlowRow
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import com.somnia.app.analysis.Bedtime
import com.somnia.app.audio.Ambience
import com.somnia.app.audio.SleepSounds
import com.somnia.app.ui.components.GlassCard
import com.somnia.app.ui.components.GradientButton
import com.somnia.app.ui.components.Segmented
import com.somnia.app.ui.components.ToggleChip
import com.somnia.app.ui.theme.TextSoft

/** Sons pour s'endormir : ambiance + minuterie, arrêt progressif ; la détection attend la fin du son. */
@OptIn(ExperimentalLayoutApi::class)
@Composable
fun SleepSoundsCard() {
    val active by SleepSounds.active.collectAsState()
    var type by remember { mutableStateOf(Ambience.RAIN) }
    var timer by remember { mutableIntStateOf(1) }
    val minutes = listOf(15, 30, 45, 60)
    GlassCard(title = "Sons pour s'endormir", icon = "🔊") {
        val a = active
        if (a != null) {
            val left = ((a.endsAt - System.currentTimeMillis()) / 60_000).coerceAtLeast(0) + 1
            Text("${a.type.label} · s'arrête doucement dans $left min", style = MaterialTheme.typography.titleSmall)
            Spacer(Modifier.height(8.dp))
            GradientButton("Arrêter le son") { SleepSounds.stop() }
        } else {
            FlowRow(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                Ambience.entries.forEach { t -> ToggleChip(t.label, t == type) { type = t } }
            }
            Spacer(Modifier.height(8.dp))
            Segmented(minutes.map { "$it min" }, timer) { timer = it }
            Spacer(Modifier.height(10.dp))
            GradientButton("Lancer ${type.label.lowercase()}") { SleepSounds.play(type, minutes[timer]) }
        }
        Text(
            "Le volume baisse sur les dernières minutes. L'écoute de la nuit attend la fin du son pour ne pas l'enregistrer.",
            style = MaterialTheme.typography.labelSmall, color = TextSoft, modifier = Modifier.padding(top = 6.dp),
        )
    }
}

/** Heure de coucher idéale pour se réveiller en fin de cycle. */
@Composable
fun IdealBedtimeCard(wakeMin: Int, fromAlarm: Boolean) {
    GlassCard(title = "Heure de coucher idéale", icon = "🛏️") {
        val wake = "%02d:%02d".format(wakeMin / 60, wakeMin % 60)
        Text(
            (if (fromAlarm) "Pour un réveil à $wake" else "Pour un réveil vers $wake (votre habitude)") + " en fin de cycle :",
            style = MaterialTheme.typography.bodySmall, color = TextSoft,
        )
        Bedtime.suggestions(wakeMin).forEach { (cycles, bed) ->
            Text(
                "%02d:%02d".format(bed / 60, bed % 60) + "  ·  $cycles cycles (${com.somnia.app.ui.Fmt.duration(cycles * Bedtime.CYCLE_MIN)} de sommeil)",
                style = MaterialTheme.typography.titleMedium, modifier = Modifier.fillMaxWidth().padding(top = 4.dp),
            )
        }
        Text("Un cycle dure environ 90 min ; on compte 15 min pour s'endormir.", style = MaterialTheme.typography.labelSmall, color = TextSoft, modifier = Modifier.padding(top = 4.dp))
    }
}
