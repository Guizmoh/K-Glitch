package com.somnia.app.alarm

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Button
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.somnia.app.service.SleepTrackingService
import com.somnia.app.ui.theme.SomniaTheme
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale

/** Écran affiché par-dessus l'écran de verrouillage quand le réveil sonne. */
class AlarmActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContent {
            SomniaTheme {
                AlarmScreen(
                    onStop = {
                        AlarmScheduler.stopRinging(this)
                        SleepTrackingService.stop(this)
                        finish()
                    },
                    onSnooze = {
                        AlarmScheduler.snooze(this)
                        finish()
                    },
                )
            }
        }
    }
}

@Composable
private fun AlarmScreen(onStop: () -> Unit, onSnooze: () -> Unit) {
    Surface(Modifier.fillMaxSize(), color = MaterialTheme.colorScheme.background) {
        Column(
            Modifier.padding(32.dp),
            verticalArrangement = Arrangement.Center,
            horizontalAlignment = Alignment.CenterHorizontally,
        ) {
            Text("☀️", fontSize = 72.sp)
            Text(
                SimpleDateFormat("HH:mm", Locale.FRANCE).format(Date()),
                fontSize = 64.sp, color = MaterialTheme.colorScheme.primary,
            )
            Text(
                "Réveil en phase de sommeil léger pour un lever plus facile.",
                textAlign = TextAlign.Center, style = MaterialTheme.typography.bodyLarge,
            )
            Spacer(Modifier.height(48.dp))
            Button(onClick = onStop, modifier = Modifier.fillMaxWidth().height(64.dp)) {
                Text("Je me lève (fin de la nuit)", fontSize = 18.sp)
            }
            Spacer(Modifier.height(16.dp))
            OutlinedButton(onClick = onSnooze, modifier = Modifier.fillMaxWidth().height(56.dp)) {
                Text("Encore 9 minutes")
            }
        }
    }
}
