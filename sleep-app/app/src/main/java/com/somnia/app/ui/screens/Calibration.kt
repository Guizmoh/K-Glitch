package com.somnia.app.ui.screens

import android.Manifest
import android.content.pm.PackageManager
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Checkbox
import androidx.compose.material3.LinearProgressIndicator
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableFloatStateOf
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp
import androidx.core.content.ContextCompat
import com.somnia.app.audio.AudioMath
import com.somnia.app.audio.MicChoice
import com.somnia.app.audio.Resample
import com.somnia.app.data.Settings
import com.somnia.app.ui.components.LevelMeter
import com.somnia.app.ui.theme.Good
import com.somnia.app.ui.theme.TextSoft
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

/** Résultat de la calibration : réglages conseillés d'après une minute d'écoute de la chambre. */
data class CalibrationResult(val floorDb: Float, val spreadDb: Float, val sensitivity: Int, val suggestTv: Boolean) {
    val roomLabel get() = when {
        floorDb < -62f -> "très calme"
        floorDb < -52f -> "calme"
        floorDb < -42f -> "un peu bruyante"
        else -> "bruyante"
    }

    companion object {
        /** [levels] : niveaux (dBFS) toutes les 100 ms. */
        fun from(levels: List<Float>): CalibrationResult {
            val s = levels.sorted()
            val floor = s[(s.size * 0.2f).toInt()]
            val p90 = s[(s.size * 0.9f).toInt().coerceAtMost(s.size - 1)]
            // Le seuil est déjà relatif au bruit de fond : on règle selon la STABILITÉ de la chambre,
            // jamais en dessous de 4 (sinon la voix n'est plus captée, surtout avec une télé).
            val spread = p90 - floor
            val sens = when {
                spread < 8f -> 6
                spread < 14f -> 5
                else -> 4
            }
            // son fort et régulier presque tout le temps (au-dessus des rares vrais silences) : télé / radio
            val silence = s[(s.size * 0.05f).toInt()]
            val active = levels.count { it > silence + 6f }.toFloat() / levels.size
            return CalibrationResult(floor, p90 - floor, sens, suggestTv = active > 0.6f && p90 - floor < 18f && floor > -60f)
        }
    }
}

/** Calibration : 1 minute de mesure de la chambre et du micro, puis réglage automatique. */
@Composable
fun CalibrationDialog(settings: Settings, onApply: (Settings) -> Unit, onClose: () -> Unit) {
    val ctx = LocalContext.current
    val scope = rememberCoroutineScope()
    var granted by remember { mutableStateOf(ContextCompat.checkSelfPermission(ctx, Manifest.permission.RECORD_AUDIO) == PackageManager.PERMISSION_GRANTED) }
    val ask = rememberLauncherForActivityResult(ActivityResultContracts.RequestPermission()) { granted = it }
    var step by remember { mutableIntStateOf(0) } // 0 prêt, 1 mesure, 2 résultat, 3 erreur
    var progress by remember { mutableFloatStateOf(0f) }
    var level by remember { mutableFloatStateOf(-90f) }
    var result by remember { mutableStateOf<CalibrationResult?>(null) }
    var tv by remember { mutableStateOf(false) }

    fun start() = scope.launch {
        step = 1
        val levels = withContext(Dispatchers.IO) {
            runCatching {
                val rec = MicChoice.open(ctx, settings.micSource)
                val factor = (rec.rate / AudioMath.SAMPLE_RATE).coerceAtLeast(1)
                val raw = ShortArray(AudioMath.FRAME * factor * rec.channels)
                val out = ArrayList<Float>()
                try {
                    rec.record.startRecording()
                    while (out.size < 600) { // 60 s
                        var read = 0
                        while (read < raw.size) { val r = rec.record.read(raw, read, raw.size - read); if (r < 0) break; read += r }
                        val mono = if (rec.channels == 1) raw else ShortArray(raw.size / rec.channels) { i -> (raw[i * 2] / 2 + raw[i * 2 + 1] / 2).toShort() }
                        val db = AudioMath.rmsDb(Resample.down(mono, factor))
                        out += db; level = db; progress = out.size / 600f
                    }
                } finally { rec.release() }
                out
            }.getOrNull()
        }
        if (levels == null || levels.size < 100) { step = 3; return@launch }
        result = CalibrationResult.from(levels).also { tv = it.suggestTv }
        step = 2
    }

    AlertDialog(
        onDismissRequest = { if (step != 1) onClose() },
        title = { Text("Calibration de la chambre") },
        text = {
            Column {
                when (step) {
                    0 -> Text(
                        "Posez le téléphone à sa place pour la nuit et restez silencieuse 1 minute (la télé peut rester allumée si vous la laissez la nuit). L'appli mesure la chambre et règle la sensibilité toute seule.",
                        color = TextSoft,
                    )
                    1 -> {
                        Text("Mesure en cours… ${(60 * (1 - progress)).toInt()} s")
                        Spacer(Modifier.height(8.dp))
                        LinearProgressIndicator({ progress }, Modifier.fillMaxWidth())
                        Spacer(Modifier.height(8.dp))
                        LevelMeter(level, level - 5f, level + 10f, false)
                    }
                    2 -> result?.let { r ->
                        Text("Chambre ${r.roomLabel} (bruit de fond %.0f dBFS).".format(r.floorDb), color = Good)
                        Text("Sensibilité du micro conseillée : ${r.sensitivity}/10", style = MaterialTheme.typography.titleSmall)
                        if (r.suggestTv) Text("Un son continu a été entendu (télé, radio ou ventilateur).", style = MaterialTheme.typography.bodySmall, color = TextSoft)
                        Row(verticalAlignment = Alignment.CenterVertically) {
                            Checkbox(tv, { tv = it }); Text("Activer le mode télé")
                        }
                    }
                    else -> Text("Mesure impossible : vérifiez l'accès au micro.", color = MaterialTheme.colorScheme.error)
                }
            }
        },
        confirmButton = {
            when (step) {
                0, 3 -> TextButton(onClick = { if (granted) start() else ask.launch(Manifest.permission.RECORD_AUDIO) }) { Text("Commencer (1 min)") }
                2 -> TextButton(onClick = {
                    result?.let { r -> onApply(settings.copy(micSensitivity = r.sensitivity, tvMode = tv)) }
                    onClose()
                }) { Text("Appliquer") }
                else -> Unit
            }
        },
        dismissButton = { if (step != 1) TextButton(onClick = onClose) { Text(if (step == 2) "Ignorer" else "Plus tard") } },
    )
}
