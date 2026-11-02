package com.somnia.app.ui.screens

import android.Manifest
import android.content.pm.PackageManager
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.height
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableFloatStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp
import androidx.core.content.ContextCompat
import com.somnia.app.audio.AudioMath
import com.somnia.app.audio.MicChoice
import com.somnia.app.ui.components.LevelMeter
import com.somnia.app.ui.theme.TextSoft
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.isActive
import kotlinx.coroutines.withContext

/** Fenêtre « Tester ce micro » : niveau sonore en direct + micro réellement utilisé. */
@Composable
fun MicTestDialog(micKey: String, label: String, onClose: () -> Unit) {
    val ctx = LocalContext.current
    var granted by remember {
        mutableStateOf(ContextCompat.checkSelfPermission(ctx, Manifest.permission.RECORD_AUDIO) == PackageManager.PERMISSION_GRANTED)
    }
    val ask = rememberLauncherForActivityResult(ActivityResultContracts.RequestPermission()) { granted = it }
    var level by remember { mutableFloatStateOf(-90f) }
    var peak by remember { mutableFloatStateOf(-90f) }
    var info by remember { mutableStateOf("Démarrage…") }
    var error by remember { mutableStateOf<String?>(null) }

    LaunchedEffect(granted, micKey) {
        if (!granted) { ask.launch(Manifest.permission.RECORD_AUDIO); return@LaunchedEffect }
        withContext(Dispatchers.IO) {
            val rec = runCatching { MicChoice.open(ctx, micKey) }.getOrElse { error = "Impossible d'ouvrir ce micro."; return@withContext }
            try {
                rec.record.startRecording()
                if (rec.record.recordingState != android.media.AudioRecord.RECORDSTATE_RECORDING) {
                    error = "Le micro est occupé (une nuit est peut-être en cours)."
                    return@withContext
                }
                info = rec.describe()
                val raw = ShortArray(AudioMath.FRAME * rec.channels)
                while (isActive) {
                    val n = rec.record.read(raw, 0, raw.size)
                    if (n <= 0) continue
                    val db = AudioMath.rmsDb(raw, 0, n)
                    level = db
                    peak = maxOf(peak - 0.5f, db)
                }
            } finally {
                rec.release()
            }
        }
    }

    AlertDialog(
        onDismissRequest = onClose,
        title = { Text("Test : $label") },
        text = {
            Column {
                if (error != null) Text(error!!, color = MaterialTheme.colorScheme.error)
                else {
                    Text("Parlez ou grattez près du micro : la barre doit bouger.", color = TextSoft)
                    Spacer(Modifier.height(12.dp))
                    LevelMeter(level, -70f, peak, false)
                    Spacer(Modifier.height(8.dp))
                    Text("Niveau : %.0f dB".format(level), style = MaterialTheme.typography.bodySmall)
                    Text("Micro utilisé : $info", style = MaterialTheme.typography.bodySmall, color = TextSoft)
                }
            }
        },
        confirmButton = { TextButton(onClick = onClose) { Text("Fermer") } },
    )
}
