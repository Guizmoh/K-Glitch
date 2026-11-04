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
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import kotlinx.coroutines.delay
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.somnia.app.service.SleepTrackingService
import com.somnia.app.ui.components.GradientButton
import com.somnia.app.ui.components.NightBackground
import com.somnia.app.ui.components.PoliceLogo
import com.somnia.app.ui.theme.SomniaTheme
import com.somnia.app.ui.theme.SunriseBrush
import com.somnia.app.ui.theme.TextSoft
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale

/** Écran affiché par-dessus l'écran de verrouillage quand le réveil sonne. */
class AlarmActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContent {
            SomniaTheme(forceNight = true) {
                androidx.compose.runtime.CompositionLocalProvider(
                    com.somnia.app.ui.LocalAgent provides com.somnia.app.data.SettingsRepository.get(this).settings.value.agent,
                    com.somnia.app.ui.LocalMoods provides com.somnia.app.data.SettingsRepository.get(this).settings.value.moods,
                ) {
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
}

@Composable
internal fun AlarmScreen(onStop: () -> Unit, onSnooze: () -> Unit, startAwake: Boolean = false) {
    // Le policier dort encore une seconde... puis ouvre les yeux.
    var awake by remember { mutableStateOf(startAwake) }
    LaunchedEffect(Unit) { delay(900); awake = true }
    NightBackground {
        Column(
            Modifier.fillMaxSize().padding(32.dp),
            verticalArrangement = Arrangement.Center,
            horizontalAlignment = Alignment.CenterHorizontally,
        ) {
            PoliceLogo(150.dp, awake = awake)
            Spacer(Modifier.height(24.dp))
            Text(
                SimpleDateFormat("HH:mm", Locale.FRANCE).format(Date()),
                fontSize = 72.sp, fontWeight = FontWeight.Bold, color = Color.White,
            )
            Text("Debout, l'agent vous réveille !", style = MaterialTheme.typography.titleLarge, textAlign = TextAlign.Center)
            Text(
                "Vous étiez en sommeil léger : c'est le meilleur moment pour se lever.",
                textAlign = TextAlign.Center, color = TextSoft, modifier = Modifier.padding(top = 8.dp),
            )
            Spacer(Modifier.height(48.dp))
            GradientButton("Je me lève", brush = SunriseBrush, textColor = Color(0xFF2A1A00), onClick = onStop)
            Spacer(Modifier.height(14.dp))
            GradientButton("Encore 9 minutes", onClick = onSnooze)
        }
    }
}
