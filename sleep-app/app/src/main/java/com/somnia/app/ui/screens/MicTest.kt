package com.somnia.app.ui.screens

import android.Manifest
import android.content.pm.PackageManager
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Slider
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableFloatStateOf
import androidx.compose.runtime.mutableStateListOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.core.content.ContextCompat
import com.somnia.app.audio.AudioMath
import com.somnia.app.audio.CapturedEvent
import com.somnia.app.audio.EventClassifier
import com.somnia.app.audio.HeuristicClassifier
import com.somnia.app.audio.MicChoice
import com.somnia.app.audio.SoundEventDetector
import com.somnia.app.audio.SoundType
import com.somnia.app.audio.YamnetClassifier
import com.somnia.app.data.Settings
import com.somnia.app.ui.components.LevelMeter
import com.somnia.app.ui.theme.Good
import com.somnia.app.ui.theme.TextSoft
import com.somnia.app.ui.theme.color
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.channels.Channel
import kotlinx.coroutines.isActive
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import kotlin.math.roundToInt

private class TestResult(val label: String, val kept: Boolean, val color: Color)

/**
 * Test en direct, sans rien enregistrer : on voit le niveau, le bruit de fond,
 * le seuil, et chaque son qui déclenche est classé comme pendant la nuit
 * (« gardé » ou « ignoré »). La sensibilité se règle pendant le test.
 */
@Composable
fun MicTestDialog(settings: Settings, label: String, onSensitivity: (Int) -> Unit, onClose: () -> Unit) {
    val ctx = LocalContext.current
    var granted by remember {
        mutableStateOf(ContextCompat.checkSelfPermission(ctx, Manifest.permission.RECORD_AUDIO) == PackageManager.PERMISSION_GRANTED)
    }
    val ask = rememberLauncherForActivityResult(ActivityResultContracts.RequestPermission()) { granted = it }
    var level by remember { mutableFloatStateOf(-90f) }
    var floor by remember { mutableFloatStateOf(-60f) }
    var threshold by remember { mutableFloatStateOf(-50f) }
    var capturing by remember { mutableStateOf(false) }
    var info by remember { mutableStateOf("Démarrage…") }
    var error by remember { mutableStateOf<String?>(null) }
    var sens by remember { mutableFloatStateOf(settings.micSensitivity.toFloat()) }
    val results = remember { mutableStateListOf<TestResult>() }
    val detector = remember { SoundEventDetector().apply { setSensitivity(settings.micSensitivity) } }
    val tv = remember { com.somnia.app.audio.TvDetector() }
    var tvOn by remember { mutableStateOf(false) }

    LaunchedEffect(granted) {
        if (!granted) { ask.launch(Manifest.permission.RECORD_AUDIO); return@LaunchedEffect }
        val events = Channel<CapturedEvent>(Channel.UNLIMITED)
        // Classification en arrière-plan, comme la nuit
        launch(Dispatchers.Default) {
            val c = YamnetClassifier.create(ctx) ?: HeuristicClassifier()
            val ec = EventClassifier(c, settings.thresholds())
            try {
                for (ev in events) {
                    val cls = ec.classify(ev.pcm)
                    var kept = cls.type != SoundType.NOISE && cls.type in settings.recordTypes
                    val secs = "%.1f s".format(ev.durationMs / 1000f)
                    var text = if (cls.type == SoundType.NOISE) "Bruit non identifié · $secs" else "${cls.type.label} · $secs · ${(cls.confidence * 100).roundToInt()} %"
                    // Paroles : est-ce votre voix ?
                    val print = com.somnia.app.transcribe.VoiceId.voiceprint(ctx)
                    if (cls.type == SoundType.SPEECH && print != null && settings.onlyMyVoice) {
                        val v = runCatching { com.somnia.app.transcribe.VoiceId.vector(ctx, ev.pcm) }.getOrNull()
                        if (v != null) {
                            val sim = com.somnia.app.transcribe.VoiceId.cosine(print, v)
                            val mine = sim >= settings.voiceMatch
                            text = (if (mine) "Votre voix" else "Autre voix (télé ?)") + " · $secs · ressemblance ${(sim * 100).roundToInt()} %"
                            kept = kept && mine
                        }
                    }
                    withContext(Dispatchers.Main) {
                        results.add(0, TestResult(text, kept, cls.type.color()))
                        while (results.size > 5) results.removeAt(results.lastIndex)
                    }
                }
            } finally { c.close() }
        }
        withContext(Dispatchers.IO) {
            val rec = runCatching { MicChoice.open(ctx, settings.micSource) }.getOrElse { error = "Impossible d'ouvrir ce micro."; return@withContext }
            try {
                rec.record.startRecording()
                if (rec.record.recordingState != android.media.AudioRecord.RECORDSTATE_RECORDING) {
                    error = "Le micro est occupé (une nuit est peut-être en cours)."; return@withContext
                }
                info = rec.describe()
                val raw = ShortArray(AudioMath.FRAME * rec.channels)
                var tick = 0
                while (isActive) {
                    var read = 0
                    while (read < raw.size && isActive) {
                        val r = rec.record.read(raw, read, raw.size - read); if (r < 0) break; read += r
                    }
                    if (read < raw.size) continue
                    val frame = if (rec.channels == 1) raw.copyOf() else ShortArray(AudioMath.FRAME) { i ->
                        var sum = 0; for (c in 0 until rec.channels) sum += raw[i * rec.channels + c]; (sum / rec.channels).toShort()
                    }
                    if (settings.tvMode) {
                        tv.process(AudioMath.rmsDb(frame), detector.noiseFloorDb)
                        detector.mediaLevelDb = if (tv.isOn) tv.tvLevelDb else -96f
                        tvOn = tv.isOn
                    }
                    detector.process(frame)?.let { events.trySend(it) }
                    if (++tick % 2 == 0) {
                        level = detector.lastDb; floor = detector.noiseFloorDb
                        threshold = detector.threshold; capturing = detector.isRecording
                    }
                }
            } finally { rec.release(); events.close() }
        }
    }

    AlertDialog(
        onDismissRequest = onClose,
        title = { Text("Tester la détection") },
        text = {
            Column {
                if (error != null) { Text(error!!, color = MaterialTheme.colorScheme.error); return@Column }
                Text("Rien n'est enregistré. Bougez les draps, parlez, toussez… et regardez ce qui déclenche.", color = TextSoft, style = MaterialTheme.typography.bodySmall)
                Spacer(Modifier.height(10.dp))
                LevelMeter(level, floor, threshold, capturing)
                Text(
                    if (capturing) "Déclenché : ce son serait analysé" else "En écoute…",
                    color = if (capturing) Color(0xFFE08A7E) else TextSoft, fontWeight = FontWeight.SemiBold,
                    modifier = Modifier.padding(top = 6.dp),
                )
                Text("Gris : bruit de fond · Jaune : seuil", style = MaterialTheme.typography.labelSmall, color = TextSoft)
                if (tvOn) Text("Télé détectée : le seuil passe au-dessus de son volume", color = Good, style = MaterialTheme.typography.bodySmall)
                else if (settings.tvMode) Text("Mode télé actif (une télé est repérée après ~1 à 2 min de son continu)", color = TextSoft, style = MaterialTheme.typography.labelSmall)
                Spacer(Modifier.height(10.dp))
                Text("Sensibilité du micro : ${sens.roundToInt()}/10", style = MaterialTheme.typography.titleSmall)
                Slider(sens, {
                    sens = it; detector.setSensitivity(it.roundToInt()); onSensitivity(it.roundToInt())
                }, valueRange = 1f..10f, steps = 8)
                Text("Derniers déclenchements", style = MaterialTheme.typography.titleSmall)
                if (results.isEmpty()) Text("Aucun pour l'instant.", color = TextSoft, style = MaterialTheme.typography.bodySmall)
                results.forEach { r ->
                    Row(Modifier.fillMaxWidth().padding(vertical = 3.dp), verticalAlignment = Alignment.CenterVertically) {
                        Box(Modifier.clip(RoundedCornerShape(6.dp)).background(r.color.copy(alpha = 0.18f)).padding(horizontal = 8.dp, vertical = 4.dp)) {
                            Text(r.label, color = r.color, style = MaterialTheme.typography.labelMedium)
                        }
                        Spacer(Modifier.weight(1f))
                        Text(if (r.kept) "gardé" else "ignoré", color = if (r.kept) Good else TextSoft, style = MaterialTheme.typography.labelMedium)
                    }
                }
                Text("Micro : $info", style = MaterialTheme.typography.labelSmall, color = TextSoft, modifier = Modifier.padding(top = 8.dp))
            }
        },
        confirmButton = { TextButton(onClick = onClose) { Text("Fermer") } },
    )
}
