package com.somnia.app.ui

import androidx.compose.runtime.Composable
import app.cash.paparazzi.DeviceConfig
import app.cash.paparazzi.Paparazzi
import com.somnia.app.audio.SoundType
import com.somnia.app.data.Settings
import com.somnia.app.data.SleepSession
import com.somnia.app.service.LiveState
import com.somnia.app.ui.components.NightBackground
import com.somnia.app.ui.screens.TonightIdle
import com.somnia.app.ui.screens.TonightLive
import com.somnia.app.ui.theme.SomniaTheme
import org.junit.Rule
import org.junit.Test

/** Rendu des écrans principaux (images dans app/src/test/snapshots). */
class ScreenshotTest {
    @get:Rule
    val paparazzi = Paparazzi(deviceConfig = DeviceConfig.PIXEL_6, theme = "android:Theme.Material.NoActionBar", maxPercentDifference = 1.0)

    private fun shot(content: @Composable () -> Unit) = paparazzi.snapshot {
        SomniaTheme { NightBackground { content() } }
    }

    private val t0 = 1_761_950_000_000L

    @Test fun tonightIdle() = shot {
        TonightIdle(
            settings = Settings(smartAlarmEnabled = true, alarmHour = 7, alarmMinute = 15),
            lastNight = SleepSession(id = 1, startTime = t0, endTime = t0 + 8 * 3_600_000L, score = 82, deepMinutes = 95, lightMinutes = 330),
            tags = setOf("Sport", "Écrans tard"),
            onToggleTag = {}, onStart = {}, onOpen = {}, hour = 22,
        )
    }

    @Test fun tonightLive() = shot {
        TonightLive(
            LiveState(
                sessionId = 1, startTime = t0, levelDb = -38f, noiseFloorDb = -55f, thresholdDb = -44f, capturing = true,
                eventCount = 9, counts = mapOf(SoundType.SNORING to 5, SoundType.SPEECH to 2, SoundType.LAUGHTER to 1, SoundType.CRYING to 1),
                lastEvent = SoundType.SPEECH, lastEventTime = t0 + 3_000_000L, alarmTarget = t0 + 30_000_000L,
            ),
            now = t0 + 2 * 3_600_000L + 14 * 60_000L,
        ) {}
    }

    @Test fun history() = shot {
        val nights = (0 until 4).map { i ->
            SleepSession(
                id = i + 1L, startTime = t0 - i * 86_400_000L, endTime = t0 - i * 86_400_000L + 8 * 3_600_000L,
                score = listOf(82, 64, 91, 47)[i], deepMinutes = 90, lightMinutes = 300 + i * 20,
                snoringMinutes = listOf(12, 0, 35, 4)[i], awakenings = listOf(1, 3, 0, 5)[i], morningMood = listOf(4, 3, 5, 2)[i],
            )
        }
        com.somnia.app.ui.screens.HistoryContent(nights) {}
    }

    @Test fun alarm() = shot { com.somnia.app.alarm.AlarmScreen(onStop = {}, onSnooze = {}) }
}
