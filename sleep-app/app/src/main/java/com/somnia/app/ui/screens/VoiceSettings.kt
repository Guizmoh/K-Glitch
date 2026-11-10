package com.somnia.app.ui.screens

import android.Manifest
import android.content.pm.PackageManager
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.LinearProgressIndicator
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.Slider
import androidx.compose.material3.Switch
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableFloatStateOf
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.unit.dp
import androidx.core.content.ContextCompat
import com.somnia.app.audio.AudioMath
import com.somnia.app.audio.MicChoice
import com.somnia.app.data.Settings
import com.somnia.app.transcribe.Transcriber
import com.somnia.app.transcribe.VoiceId
import com.somnia.app.ui.components.GlassCard
import com.somnia.app.ui.components.LevelMeter
import com.somnia.app.ui.theme.Good
import com.somnia.app.ui.theme.TextSoft
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import kotlin.math.roundToInt

/** Carte « Télé et autres voix » : mode télé + empreinte de votre voix. */
@Composable
fun TvAndVoiceCard(s: Settings, update: ((Settings) -> Settings) -> Unit, live: Boolean) {
    val ctx = LocalContext.current
    val scope = rememberCoroutineScope()
    val tState by Transcriber.state.collectAsState()
    val spkDl by VoiceId.downloading.collectAsState()
    var refresh by remember { mutableIntStateOf(0) }
    val ready = remember(tState, spkDl, refresh) { VoiceId.isReady(ctx) }
    val hasPrint = remember(refresh) { VoiceId.hasVoiceprint(ctx) }
    var enrolling by remember { mutableStateOf(false) }
    var error by remember { mutableStateOf<String?>(null) }

    GlassCard(title = "Télé et autres voix", icon = "📺") {
        Row(verticalAlignment = Alignment.CenterVertically) {
            Column(Modifier.weight(1f)) {
                Text("Mode télé automatique")
                Text(
                    "Repère une télé ou une radio allumée (son continu, volume régulier) et la traite comme bruit de fond.",
                    style = MaterialTheme.typography.bodySmall, color = TextSoft,
                )
            }
            Switch(s.tvMode, { v -> update { it.copy(tvMode = v) } })
        }
        Spacer(Modifier.height(14.dp))
        Text("Ma voix", style = MaterialTheme.typography.titleSmall)
        Text(
            "Enregistrez votre voix une fois : quand une télé ou une radio est repérée, seules les paroles qui vous ressemblent sont gardées. Sans télé, toutes les paroles sont gardées.",
            style = MaterialTheme.typography.bodySmall, color = TextSoft,
        )
        Spacer(Modifier.height(8.dp))
        val dlT = tState as? Transcriber.State.Downloading
        when {
            dlT != null || spkDl != null -> {
                val p = dlT?.progress ?: spkDl ?: 0f
                Text(if (dlT != null) "Téléchargement (1/2)… ${(p * 100).toInt()} %" else "Téléchargement (2/2)… ${(p * 100).toInt()} %")
                LinearProgressIndicator({ p }, Modifier.fillMaxWidth().padding(top = 6.dp))
            }
            !ready -> OutlinedButton(onClick = {
                error = null
                scope.launch {
                    if (!Transcriber.isReady(ctx)) Transcriber.download(ctx)
                    if (Transcriber.isReady(ctx) && !VoiceId.isModelReady(ctx)) runCatching { VoiceId.downloadModel(ctx) }
                    if (!VoiceId.isReady(ctx)) error = "Téléchargement impossible : vérifiez la connexion Internet."
                    refresh++
                }
            }, modifier = Modifier.fillMaxWidth()) {
                Text(if (Transcriber.isReady(ctx)) "Installer la reconnaissance de voix (14 Mo)" else "Installer la reconnaissance de voix (56 Mo, Wi-Fi conseillé)")
            }
            !hasPrint -> OutlinedButton(onClick = { enrolling = true }, enabled = !live, modifier = Modifier.fillMaxWidth()) {
                Text("Enregistrer ma voix (20 secondes)")
            }
            else -> {
                Text("Empreinte vocale enregistrée.", color = Good)
                Row(verticalAlignment = Alignment.CenterVertically, modifier = Modifier.padding(top = 4.dp)) {
                    Text("Ne garder que ma voix", Modifier.weight(1f))
                    Switch(s.onlyMyVoice, { v -> update { it.copy(onlyMyVoice = v) } })
                }
                if (s.onlyMyVoice) {
                    Text("Exigence de ressemblance : ${(s.voiceMatch * 100).roundToInt()} %", Modifier.padding(top = 6.dp))
                    Slider(s.voiceMatch, { v -> update { it.copy(voiceMatch = (v * 20).roundToInt() / 20f) } }, valueRange = 0.25f..0.75f, steps = 9)
                    Text(
                        "Plus bas : garde plus facilement votre voix (même endormie). Plus haut : rejette mieux la télé.",
                        style = MaterialTheme.typography.bodySmall, color = TextSoft,
                    )
                }
                Row(Modifier.fillMaxWidth().padding(top = 6.dp)) {
                    TextButton(onClick = { enrolling = true }, enabled = !live) { Text("Refaire") }
                    TextButton(onClick = { VoiceId.forget(ctx); refresh++ }) { Text("Supprimer l'empreinte") }
                }
            }
        }
        error?.let { Text(it, color = MaterialTheme.colorScheme.error, modifier = Modifier.padding(top = 4.dp)) }
    }
    if (enrolling) EnrollDialog(s.micSource) { enrolling = false; refresh++ }
}

private const val ENROLL_TEXT =
    "Bonsoir l'agent du Dodo. Ce soir, je vais bien dormir. Je pose mon téléphone près de mon lit, " +
        "j'éteins la lumière et je ferme les yeux. Demain matin, je me réveillerai en pleine forme, " +
        "prête pour une belle journée. Bonne nuit et à demain !"

/** Enregistre 20 s de voix pour créer l'empreinte. */
@Composable
private fun EnrollDialog(micKey: String, onClose: () -> Unit) {
    val ctx = LocalContext.current
    var granted by remember {
        mutableStateOf(ContextCompat.checkSelfPermission(ctx, Manifest.permission.RECORD_AUDIO) == PackageManager.PERMISSION_GRANTED)
    }
    val ask = rememberLauncherForActivityResult(ActivityResultContracts.RequestPermission()) { granted = it }
    var step by remember { mutableIntStateOf(0) } // 0 prêt, 1 enregistre, 2 analyse, 3 fini, 4 échec
    var progress by remember { mutableFloatStateOf(0f) }
    var level by remember { mutableFloatStateOf(-90f) }
    val scope = rememberCoroutineScope()

    fun start() = scope.launch {
        step = 1
        val pcm = withContext(Dispatchers.IO) {
            val rec = MicChoice.open(ctx, micKey)
            val total = AudioMath.SAMPLE_RATE * 20
            val out = ShortArray(total)
            val raw = ShortArray(AudioMath.FRAME * rec.channels)
            var pos = 0
            try {
                rec.record.startRecording()
                while (pos < total) {
                    val n = rec.record.read(raw, 0, raw.size); if (n <= 0) continue
                    val frames = n / rec.channels
                    for (i in 0 until frames) {
                        if (pos >= total) break
                        var sum = 0; for (c in 0 until rec.channels) sum += raw[i * rec.channels + c]
                        out[pos++] = (sum / rec.channels).toShort()
                    }
                    level = AudioMath.rmsDb(raw, 0, n); progress = pos / total.toFloat()
                }
            } finally { rec.release() }
            out
        }
        step = 2
        step = if (runCatching { VoiceId.enroll(ctx, pcm) }.getOrDefault(false)) 3 else 4
    }

    AlertDialog(
        onDismissRequest = { if (step != 1 && step != 2) onClose() },
        title = { Text("Enregistrer ma voix") },
        text = {
            Column {
                when (step) {
                    0 -> Text("Mettez-vous à votre place habituelle dans le lit, puis lisez ce texte à voix normale :", color = TextSoft)
                    1 -> Text("Lisez le texte… ${(20 * (1 - progress)).roundToInt()} s", color = Good)
                    2 -> Text("Analyse de votre voix…")
                    3 -> Text("C'est enregistré ! Votre voix sera reconnue la nuit.", color = Good)
                    else -> Text("Pas assez de voix entendue. Recommencez en parlant un peu plus fort.", color = MaterialTheme.colorScheme.error)
                }
                Spacer(Modifier.height(10.dp))
                Text(
                    "« $ENROLL_TEXT »",
                    Modifier.clip(RoundedCornerShape(12.dp)).background(com.somnia.app.ui.theme.Themes.cur.text.copy(alpha = 0.1f)).padding(12.dp),
                    style = MaterialTheme.typography.bodyLarge.copy(fontFamily = FontFamily.Serif),
                )
                if (step == 1) {
                    Spacer(Modifier.height(10.dp))
                    LinearProgressIndicator({ progress }, Modifier.fillMaxWidth())
                    Spacer(Modifier.height(6.dp))
                    LevelMeter(level, -70f, -40f, false)
                }
            }
        },
        confirmButton = {
            when (step) {
                0, 4 -> TextButton(onClick = { if (granted) start() else ask.launch(Manifest.permission.RECORD_AUDIO) }) {
                    Text(if (step == 0) "Commencer" else "Recommencer")
                }
                3 -> TextButton(onClick = onClose) { Text("Terminé") }
                else -> Unit
            }
        },
        dismissButton = { if (step == 0 || step == 4) TextButton(onClick = onClose) { Text("Annuler") } },
    )
    @Suppress("UNUSED_EXPRESSION") Spacer(Modifier.width(0.dp))
}
