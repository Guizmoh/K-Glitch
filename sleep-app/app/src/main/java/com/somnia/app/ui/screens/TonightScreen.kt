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
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
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
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.somnia.app.analysis.JournalTags
import com.somnia.app.service.SleepTrackingService
import com.somnia.app.service.TrackingState
import com.somnia.app.ui.Fmt
import com.somnia.app.ui.SomniaViewModel
import com.somnia.app.ui.components.LevelMeter
import com.somnia.app.ui.components.ScoreRing
import com.somnia.app.ui.components.SectionCard
import kotlinx.coroutines.delay

@OptIn(ExperimentalLayoutApi::class)
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
            Toast.makeText(ctx, "Le micro est nécessaire pour écouter la nuit.", Toast.LENGTH_LONG).show()
        }
    }
    val start = {
        val perms = mutableListOf(Manifest.permission.RECORD_AUDIO)
        if (Build.VERSION.SDK_INT >= 33) perms += Manifest.permission.POST_NOTIFICATIONS
        permLauncher.launch(perms.toTypedArray())
    }

    LaunchedEffect(live?.sessionId) { live?.sessionId?.let { vm.applyPendingTags(it) } }

    Column(
        Modifier.verticalScroll(rememberScrollState()).padding(16.dp),
        horizontalAlignment = Alignment.CenterHorizontally,
    ) {
        val l = live
        if (l == null) {
            Text("🌙", fontSize = 64.sp)
            Text("Bonne nuit", style = MaterialTheme.typography.headlineMedium)
            Text(
                "Posez le téléphone sur la table de nuit (ou sur le matelas pour mesurer les mouvements), branché si possible, micro vers vous.",
                style = MaterialTheme.typography.bodyMedium, color = MaterialTheme.colorScheme.onSurfaceVariant,
                modifier = Modifier.padding(vertical = 12.dp),
            )
            SectionCard("Journal du soir") {
                Text("Qu'avez-vous fait aujourd'hui ?", style = MaterialTheme.typography.bodySmall)
                FlowRow(horizontalArrangement = Arrangement.spacedBy(6.dp)) {
                    JournalTags.ALL.forEach { t ->
                        FilterChip(
                            selected = t in tags,
                            onClick = { tags = if (t in tags) tags - t else tags + t; vm.pendingTags = tags },
                            label = { Text(t) },
                        )
                    }
                }
            }
            SectionCard {
                Text("Micro : sensibilité ${settings.micSensitivity}/10", style = MaterialTheme.typography.bodyMedium)
                if (settings.smartAlarmEnabled) {
                    Text(
                        "⏰ Réveil intelligent entre %02d:%02d et %02d:%02d".format(
                            ((settings.alarmHour * 60 + settings.alarmMinute - settings.alarmWindowMin + 1440) % 1440) / 60,
                            ((settings.alarmHour * 60 + settings.alarmMinute - settings.alarmWindowMin + 1440) % 1440) % 60,
                            settings.alarmHour, settings.alarmMinute,
                        ),
                    )
                }
                if (settings.startDelayMin > 0) Text("Détection après ${settings.startDelayMin} min d'endormissement")
            }
            Spacer(Modifier.height(16.dp))
            Button(onClick = start, modifier = Modifier.fillMaxWidth().height(64.dp)) {
                Text("Commencer la nuit", fontSize = 18.sp)
            }
            sessions.firstOrNull { it.endTime != null }?.let { last ->
                SectionCard("Dernière nuit", Modifier.padding(top = 16.dp)) {
                    Row(verticalAlignment = Alignment.CenterVertically) {
                        Box(contentAlignment = Alignment.Center) {
                            ScoreRing(last.score, Modifier.size(72.dp))
                            Text(last.score?.toString() ?: "–", style = MaterialTheme.typography.titleLarge)
                        }
                        Column(Modifier.padding(start = 16.dp).weight(1f)) {
                            Text(Fmt.day(last.startTime))
                            Text("Sommeil : ${Fmt.duration(last.deepMinutes + last.lightMinutes)}")
                        }
                        Button(onClick = { openSession(last.id) }) { Text("Voir") }
                    }
                }
            }
        } else {
            var now by remember { mutableStateOf(System.currentTimeMillis()) }
            LaunchedEffect(Unit) { while (true) { delay(1000); now = System.currentTimeMillis() } }
            Text("😴", fontSize = 56.sp)
            Text("Suivi en cours", style = MaterialTheme.typography.headlineMedium)
            Text("Depuis ${Fmt.time(l.startTime)} · ${Fmt.duration(((now - l.startTime) / 60_000).toInt())}")
            l.alarmTarget?.let { Text("⏰ Réveil au plus tard à ${Fmt.time(it)}", color = MaterialTheme.colorScheme.secondary) }
            if (now < l.detectionStartsAt) {
                Text("Détection dans ${((l.detectionStartsAt - now) / 60_000) + 1} min", color = MaterialTheme.colorScheme.secondary)
            }
            SectionCard("Niveau sonore") {
                LevelMeter(l.levelDb, l.noiseFloorDb, l.thresholdDb, l.capturing)
                Text(
                    "Actuel %.0f dB · fond %.0f dB · seuil %.0f dB".format(l.levelDb, l.noiseFloorDb, l.thresholdDb),
                    style = MaterialTheme.typography.bodySmall,
                )
                Text(
                    if (l.capturing) "🔴 Enregistrement d'un son…" else "Écoute du bruit de fond",
                    style = MaterialTheme.typography.bodySmall,
                )
                Text("Reconnaissance : ${l.classifierName}", style = MaterialTheme.typography.labelSmall)
            }
            SectionCard("Sons détectés : ${l.eventCount}") {
                if (l.counts.isEmpty()) Text("Rien pour l'instant.")
                l.counts.forEach { (t, n) -> Text("${t.emoji}  ${t.label} : $n") }
                l.lastEvent?.let { Text("Dernier : ${it.label} à ${Fmt.time(l.lastEventTime ?: 0)}", style = MaterialTheme.typography.bodySmall) }
            }
            Spacer(Modifier.height(16.dp))
            Button(
                onClick = { SleepTrackingService.stop(ctx); openSession(l.sessionId) },
                modifier = Modifier.fillMaxWidth().height(64.dp),
                colors = ButtonDefaults.buttonColors(containerColor = MaterialTheme.colorScheme.secondary),
            ) { Text("☀️ Je me réveille", fontSize = 18.sp) }
        }
    }
}
