package com.somnia.app.ui.screens

import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
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
import com.somnia.app.ui.components.GlassCard
import com.somnia.app.ui.components.ScreenHeader
import com.somnia.app.ui.components.Segmented
import com.somnia.app.ui.components.StatTile
import com.somnia.app.ui.components.TileRow
import com.somnia.app.ui.theme.AwakeColor
import com.somnia.app.ui.theme.Good
import com.somnia.app.ui.theme.TextSoft
import com.somnia.app.ui.theme.color
import com.somnia.app.ui.theme.scoreColor
import java.util.Calendar
import kotlin.math.roundToInt

@Composable
fun StatsScreen(vm: SomniaViewModel) {
    val all by vm.sessions.collectAsState()
    val settings by vm.settings.collectAsState()
    var period by remember { mutableIntStateOf(0) }
    val days = listOf(7, 30, 90)[period]
    var events by remember { mutableStateOf<List<SoundEventEntity>>(emptyList()) }
    LaunchedEffect(all) { events = vm.dao.allEvents() }

    val since = System.currentTimeMillis() - days * 86_400_000L
    val nights = all.filter { it.endTime != null && it.startTime >= since }.sortedBy { it.startTime }
    val labels = nights.map { Fmt.shortDay(it.startTime).take(if (nights.size > 10) 2 else 6) }

    Column(Modifier.verticalScroll(rememberScrollState()).padding(horizontal = 20.dp)) {
        ScreenHeader("Tendances", "Vos nuits en un coup d'œil")
        com.somnia.app.analysis.WeeklyReport.compute(all, System.currentTimeMillis())?.let { w ->
            GlassCard(title = "Bilan de la semaine", icon = "⭐") {
                Text(w.summary(), style = MaterialTheme.typography.bodyMedium)
                Text(
                    "Coucher moyen %02d:%02d · écart ±${w.bedtimeSpreadMin} min".format(w.avgBedtimeMin / 60, w.avgBedtimeMin % 60),
                    style = MaterialTheme.typography.bodySmall, color = TextSoft,
                )
                w.best?.let { Text("Meilleure nuit : ${Fmt.day(it.startTime)} (${it.score})", style = MaterialTheme.typography.bodySmall, color = Good) }
                w.worst?.let { Text("Nuit la plus difficile : ${Fmt.day(it.startTime)} (${it.score})", style = MaterialTheme.typography.bodySmall, color = AwakeColor) }
                w.helped.forEach { Text("A aidé : ${it.tag} (+${it.delta} points)", style = MaterialTheme.typography.bodySmall, color = Good) }
                w.hurt.forEach { Text("A gêné : ${it.tag} (${it.delta} points)", style = MaterialTheme.typography.bodySmall, color = AwakeColor) }
                if (w.helped.isEmpty() && w.hurt.isEmpty()) Text("Remplissez le rapport du soir pour savoir ce qui aide.", style = MaterialTheme.typography.labelSmall, color = TextSoft)
            }
        }
        Segmented(listOf("7 jours", "30 jours", "90 jours"), period) { period = it }
        Spacer(Modifier.height(8.dp))
        if (nights.isEmpty()) {
            GlassCard { Text("Pas encore de nuit sur cette période.", color = TextSoft) }
            return@Column
        }

        val asleep = nights.map { it.deepMinutes + it.lightMinutes }
        val scored = nights.mapNotNull { it.score }
        val avgScore = scored.takeIf { it.isNotEmpty() }?.average()?.roundToInt()
        TileRow {
            StatTile("⭐", avgScore?.toString() ?: "–", "Score moyen", Modifier.weight(1f), accent = scoreColor(avgScore))
            StatTile("😴", Fmt.duration(asleep.average().roundToInt()), "Sommeil moyen", Modifier.weight(1f))
        }
        val bed = nights.map {
            val c = Calendar.getInstance().apply { timeInMillis = it.startTime }
            (c.get(Calendar.HOUR_OF_DAY) * 60 + c.get(Calendar.MINUTE) + 720) % 1440
        }
        val avgBed = (bed.average().roundToInt() + 720) % 1440
        TileRow {
            StatTile("🛏️", "%02d:%02d".format(avgBed / 60, avgBed % 60), "Coucher moyen", Modifier.weight(1f))
            StatTile("🐻", "${nights.map { it.snoringMinutes }.average().roundToInt()} min", "Ronflement / nuit", Modifier.weight(1f), accent = SoundType.SNORING.color())
        }
        val spread = if (bed.size > 1) bed.max() - bed.min() else 0
        if (spread > 90) GlassCard {
            Text("Horaires irréguliers (écart de ${Fmt.duration(spread)}). Un rythme régulier aide à mieux dormir.", color = AwakeColor)
        }

        GlassCard(title = "Durée de sommeil (h)", icon = "😴") {
            BarChart(asleep.map { it / 60f }, labels, maxValue = maxOf(10f, asleep.max() / 60f), goal = settings.sleepGoalMin / 60f)
            Text("Trait blanc : objectif de ${Fmt.duration(settings.sleepGoalMin)}", style = MaterialTheme.typography.labelSmall, color = TextSoft)
        }
        if (scored.isNotEmpty()) GlassCard(title = "Score", icon = "⭐") {
            BarChart(nights.map { (it.score ?: 0).toFloat() }, labels, maxValue = 100f, color = Good)
        }
        GlassCard(title = "Sommeil profond (min)", icon = "🌊") {
            BarChart(nights.map { it.deepMinutes.toFloat() }, labels, color = Color(0xFF9C9CFF))
        }
        GlassCard(title = "Ronflement (min)", icon = "🐻") {
            BarChart(nights.map { it.snoringMinutes.toFloat() }, labels, color = SoundType.SNORING.color())
        }
        val ids = nights.map { it.id }.toSet()
        val periodEvents = events.filter { it.sessionId in ids }.groupingBy { SoundType.fromName(it.type) }.eachCount()
        if (periodEvents.isNotEmpty()) GlassCard(title = "Sons relevés", icon = "🚨") {
            periodEvents.entries.sortedByDescending { it.value }.forEach { (t, n) ->
                Text("${t.label} : $n  (${"%.1f".format(n.toFloat() / nights.size)} / nuit)", Modifier.padding(vertical = 2.dp))
            }
        }
        val effects = Insights.tagEffects(all)
        GlassCard(title = "L'enquête : ce qui influence vos nuits", icon = "🔎") {
            if (effects.isEmpty()) Text("Remplissez le rapport du soir quelques nuits pour voir apparaître des pistes.", color = TextSoft)
            effects.forEach {
                val good = it.delta >= 0
                Text(
                    "${if (good) "" else ""} ${it.tag} : ${if (good) "+" else ""}${it.delta} points (${it.nights} nuits)",
                    color = if (good) Good else AwakeColor, modifier = Modifier.padding(vertical = 2.dp),
                )
            }
        }
        Spacer(Modifier.height(16.dp))
    }
}
