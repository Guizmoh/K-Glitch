package com.somnia.app.ui.screens

import android.Manifest
import android.os.Build
import android.widget.Toast
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.contract.ActivityResultContracts
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
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.somnia.app.analysis.JournalTags
import com.somnia.app.data.CapturePreset
import com.somnia.app.data.Settings
import com.somnia.app.data.SleepSession
import com.somnia.app.service.LiveState
import com.somnia.app.service.SleepTrackingService
import com.somnia.app.service.TrackingState
import com.somnia.app.ui.Fmt
import com.somnia.app.ui.SomniaViewModel
import com.somnia.app.ui.components.BigRoundButton
import com.somnia.app.ui.components.GlassCard
import com.somnia.app.ui.components.GradientButton
import com.somnia.app.ui.components.LevelMeter
import com.somnia.app.ui.components.ListeningOrb
import com.somnia.app.ui.components.Pill
import com.somnia.app.ui.components.PoliceLogo
import com.somnia.app.ui.components.ScoreRing
import com.somnia.app.ui.components.ScreenHeader
import com.somnia.app.ui.components.ToggleChip
import com.somnia.app.ui.theme.Badge
import com.somnia.app.ui.theme.Moon
import com.somnia.app.ui.theme.SunriseBrush
import com.somnia.app.ui.theme.TextSoft
import com.somnia.app.ui.theme.color
import com.somnia.app.ui.theme.scoreColor
import kotlinx.coroutines.delay
import java.util.Calendar

@Composable
fun TonightScreen(vm: SomniaViewModel, openSession: (Long) -> Unit) {
    val ctx = LocalContext.current
    val live by TrackingState.state.collectAsState()
    val settings by vm.settings.collectAsState()
    val sessions by vm.sessions.collectAsState()
    var tags by remember { mutableStateOf(vm.pendingTags) }

    val permLauncher = rememberLauncherForActivityResult(ActivityResultContracts.RequestMultiplePermissions()) { res ->
        if (res[Manifest.permission.RECORD_AUDIO] == true) {
            vm.pendingTags = tags
            SleepTrackingService.start(ctx)
        } else {
            Toast.makeText(ctx, "Le micro est nécessaire pour surveiller la nuit.", Toast.LENGTH_LONG).show()
        }
    }
    LaunchedEffect(live?.sessionId) { live?.sessionId?.let { vm.applyPendingTags(it) } }

    val l = live
    if (l == null) {
        TonightIdle(
            settings = settings,
            lastNight = sessions.firstOrNull { it.endTime != null },
            tags = tags,
            onToggleTag = { t -> tags = if (t in tags) tags - t else tags + t; vm.pendingTags = tags },
            onStart = {
                val perms = mutableListOf(Manifest.permission.RECORD_AUDIO)
                if (Build.VERSION.SDK_INT >= 33) perms += Manifest.permission.POST_NOTIFICATIONS
                permLauncher.launch(perms.toTypedArray())
            },
            onOpen = openSession,
        )
    } else {
        var now by remember { mutableStateOf(System.currentTimeMillis()) }
        LaunchedEffect(Unit) { while (true) { delay(1000); now = System.currentTimeMillis() } }
        TonightLive(l, now) { SleepTrackingService.stop(ctx); openSession(l.sessionId) }
    }
}

@OptIn(ExperimentalLayoutApi::class)
@Composable
fun TonightIdle(
    settings: Settings,
    lastNight: SleepSession?,
    tags: Set<String>,
    onToggleTag: (String) -> Unit,
    onStart: () -> Unit,
    onOpen: (Long) -> Unit,
    hour: Int = Calendar.getInstance().get(Calendar.HOUR_OF_DAY),
) {
    Column(
        Modifier.verticalScroll(rememberScrollState()).padding(horizontal = 20.dp),
        horizontalAlignment = Alignment.CenterHorizontally,
    ) {
        val hello = when (hour) { in 5..11 -> "Bonjour"; in 12..17 -> "Bon après-midi"; else -> "Bonsoir" }
        ScreenHeader("Police du Dodo", "$hello ! On surveille votre nuit ?")

        BigRoundButton("Commencer", "la nuit", onClick = onStart)

        FlowRow(horizontalArrangement = Arrangement.spacedBy(8.dp, Alignment.CenterHorizontally), modifier = Modifier.fillMaxWidth()) {
            Pill("🎙️ ${CapturePreset.of(settings)?.label ?: "Personnalisé"}", Moon, Modifier.padding(bottom = 6.dp))
            if (settings.smartAlarmEnabled) {
                Pill("⏰ %02d:%02d".format(settings.alarmHour, settings.alarmMinute), Badge, Modifier.padding(bottom = 6.dp))
            }
            if (settings.startDelayMin > 0) Pill("⏳ ${settings.startDelayMin} min", TextSoft, Modifier.padding(bottom = 6.dp))
        }
        Spacer(Modifier.height(8.dp))

        GlassCard(title = "Rapport du soir", icon = "📝") {
            Text("Facultatif : ça aide à comprendre ce qui influence vos nuits.", style = MaterialTheme.typography.bodySmall, color = TextSoft)
            Spacer(Modifier.height(8.dp))
            FlowRow(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                JournalTags.ALL.forEach { t -> ToggleChip(t, t in tags) { onToggleTag(t) } }
            }
        }

        lastNight?.let { last ->
            GlassCard(onClick = { onOpen(last.id) }) {
                Row(verticalAlignment = Alignment.CenterVertically) {
                    Box(contentAlignment = Alignment.Center) {
                        ScoreRing(last.score, Modifier.size(64.dp))
                        Text(last.score?.toString() ?: "–", style = MaterialTheme.typography.titleLarge, color = scoreColor(last.score))
                    }
                    Column(Modifier.padding(start = 16.dp).weight(1f)) {
                        Text("Dernière nuit", style = MaterialTheme.typography.titleMedium)
                        Text(
                            "${Fmt.duration(last.deepMinutes + last.lightMinutes)} de sommeil · ${Fmt.shortDay(last.startTime)}",
                            style = MaterialTheme.typography.bodySmall, color = TextSoft,
                        )
                    }
                    Text("›", fontSize = 28.sp, color = TextSoft)
                }
            }
        }
        GlassCard {
            Text(
                "💡 Posez le téléphone près de votre tête, branché. Sur le matelas, il mesure aussi vos mouvements.",
                style = MaterialTheme.typography.bodySmall, color = TextSoft,
            )
        }
        Spacer(Modifier.height(12.dp))
    }
}

@OptIn(ExperimentalLayoutApi::class)
@Composable
fun TonightLive(l: LiveState, now: Long, onStop: () -> Unit) {
    Column(
        Modifier.verticalScroll(rememberScrollState()).padding(horizontal = 20.dp),
        horizontalAlignment = Alignment.CenterHorizontally,
    ) {
        ScreenHeader("Surveillance en cours", "Depuis ${Fmt.time(l.startTime)} · tout reste sur le téléphone")

        ListeningOrb(l.levelDb, l.noiseFloorDb, l.capturing) {
            Column(horizontalAlignment = Alignment.CenterHorizontally) {
                PoliceLogo(58.dp, awake = false)
                Text(Fmt.duration(((now - l.startTime) / 60_000).toInt()), fontSize = 34.sp, fontWeight = FontWeight.Bold, color = Color.White)
                Text(
                    when {
                        now < l.detectionStartsAt -> "Endormissement… ${((l.detectionStartsAt - now) / 60_000) + 1} min"
                        l.capturing -> "🔴 J'enregistre"
                        else -> "J'écoute…"
                    },
                    color = Color.White.copy(alpha = 0.85f), textAlign = TextAlign.Center,
                )
            }
        }

        l.alarmTarget?.let { Pill("⏰ Réveil au plus tard à ${Fmt.time(it)}", Badge) }
        Spacer(Modifier.height(8.dp))

        GlassCard(title = "Sons relevés : ${l.eventCount}", icon = "🚨") {
            if (l.counts.isEmpty()) Text("Rien à signaler pour l'instant.", color = TextSoft)
            FlowRow(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                l.counts.forEach { (t, n) -> Pill("${t.emoji} ${t.label} · $n", t.color(), Modifier.padding(bottom = 6.dp)) }
            }
            l.lastEvent?.let {
                Text("Dernier : ${it.label.lowercase()} à ${Fmt.time(l.lastEventTime ?: 0)}", style = MaterialTheme.typography.bodySmall, color = TextSoft)
            }
        }
        GlassCard(title = "Niveau sonore", icon = "🎚️") {
            LevelMeter(l.levelDb, l.noiseFloorDb, l.thresholdDb, l.capturing)
            Spacer(Modifier.height(6.dp))
            Text("Gris : bruit de fond · Jaune : seuil de déclenchement", style = MaterialTheme.typography.labelSmall, color = TextSoft)
        }
        Spacer(Modifier.height(12.dp))
        GradientButton("☀️  Je me réveille", brush = SunriseBrush, textColor = Color(0xFF2A1A00), onClick = onStop)
        Spacer(Modifier.height(16.dp))
    }
}
