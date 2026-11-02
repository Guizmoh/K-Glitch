package com.somnia.app.ui.screens

import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.FilterChip
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.dp
import com.somnia.app.analysis.Insights
import com.somnia.app.audio.SoundType
import com.somnia.app.data.SoundEventEntity
import com.somnia.app.ui.Fmt
import com.somnia.app.ui.SomniaViewModel
import com.somnia.app.ui.components.BarChart
import com.somnia.app.ui.components.SectionCard
import com.somnia.app.ui.theme.DeepColor
import com.somnia.app.ui.theme.color
import kotlin.math.roundToInt

@Composable
fun StatsScreen(vm: SomniaViewModel) {
    val all by vm.sessions.collectAsState()
    val settings by vm.settings.collectAsState()
    var days by remember { mutableStateOf(7) }
    var events by remember { mutableStateOf<List<SoundEventEntity>>(emptyList()) }
    LaunchedEffect(all) { events = vm.dao.allEvents() }

    val since = System.currentTimeMillis() - days * 86_400_000L
    val nights = all.filter { it.endTime != null && it.startTime >= since }.sortedBy { it.startTime }
    val labels = nights.map { Fmt.shortDay(it.startTime).take(if (nights.size > 10) 2 else 6) }

    Column(Modifier.verticalScroll(rememberScrollState()).padding(16.dp)) {
        Text("Tendances", style = MaterialTheme.typography.headlineSmall)
        Row {
            listOf(7, 30, 90).forEach { d ->
                FilterChip(days == d, { days = d }, { Text("$d jours") }, Modifier.padding(end = 6.dp))
            }
        }
        if (nights.isEmpty()) { Text("Pas encore de nuit sur cette période."); return@Column }

        val asleep = nights.map { it.deepMinutes + it.lightMinutes }
        val scored = nights.mapNotNull { it.score }
        SectionCard("Résumé") {
            Text("Durée moyenne : ${Fmt.duration(asleep.average().roundToInt())} (objectif ${Fmt.duration(settings.sleepGoalMin)})")
            if (scored.isNotEmpty()) Text("Score moyen : ${scored.average().roundToInt()} / 100")
            Text("Ronflement moyen : ${nights.map { it.snoringMinutes }.average().roundToInt()} min / nuit")
            Text("Réveils moyens : ${"%.1f".format(nights.map { it.awakenings }.average())} / nuit")
            val bed = nights.map { val c = java.util.Calendar.getInstance().apply { timeInMillis = it.startTime }
                (c.get(java.util.Calendar.HOUR_OF_DAY) * 60 + c.get(java.util.Calendar.MINUTE) + 720) % 1440 }
            val avgBed = (bed.average().roundToInt() + 720) % 1440
            Text("Coucher moyen : %02d:%02d".format(avgBed / 60, avgBed % 60))
            val spread = if (bed.size > 1) bed.max() - bed.min() else 0
            if (spread > 90) Text("⚠️ Horaires irréguliers (écart ${Fmt.duration(spread)}) : un rythme régulier améliore le sommeil.", style = MaterialTheme.typography.bodySmall)
        }
        SectionCard("Durée de sommeil") {
            BarChart(asleep.map { it / 60f }, labels, maxValue = maxOf(10f, asleep.max() / 60f), goal = settings.sleepGoalMin / 60f)
        }
        if (scored.isNotEmpty()) SectionCard("Score") {
            BarChart(nights.map { (it.score ?: 0).toFloat() }, labels, maxValue = 100f, color = Color(0xFF7DDE92))
        }
        SectionCard("Sommeil profond (min)") {
            BarChart(nights.map { it.deepMinutes.toFloat() }, labels, color = DeepColor)
        }
        SectionCard("Ronflement (min)") {
            BarChart(nights.map { it.snoringMinutes.toFloat() }, labels, color = SoundType.SNORING.color())
        }
        val ids = nights.map { it.id }.toSet()
        val periodEvents = events.filter { it.sessionId in ids }.groupingBy { SoundType.fromName(it.type) }.eachCount()
        if (periodEvents.isNotEmpty()) SectionCard("Sons de la période") {
            periodEvents.entries.sortedByDescending { it.value }.forEach { (t, n) ->
                Text("${t.emoji}  ${t.label} : $n (${"%.1f".format(n.toFloat() / nights.size)} / nuit)")
            }
        }
        val effects = Insights.tagEffects(all)
        SectionCard("Ce qui influence vos nuits") {
            if (effects.isEmpty()) Text("Remplissez le journal du soir quelques nuits pour voir apparaître des tendances.")
            effects.forEach {
                val sign = if (it.delta >= 0) "+" else ""
                Text("${it.tag} : score $sign${it.delta} (${it.withAvg} vs ${it.withoutAvg}, ${it.nights} nuits)")
            }
        }
    }
}
