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
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.ui.unit.dp
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
            tags = setOf("Stress", "Écrans tard"),
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
        com.somnia.app.ui.screens.HistoryContent(nights.mapIndexed { i, n -> n.copy(favorite = i == 0 || i == 2) }, open = {})
    }

    @Test fun alarm() = shot { com.somnia.app.alarm.AlarmScreen(onStop = {}, onSnooze = {}, startAwake = true) }

    @Test fun logos() = paparazzi.snapshot {
        SomniaTheme {
            NightBackground {
                androidx.compose.foundation.layout.Row(
                    androidx.compose.ui.Modifier.padding(40.dp),
                    horizontalArrangement = androidx.compose.foundation.layout.Arrangement.spacedBy(40.dp),
                ) {
                    com.somnia.app.ui.components.PoliceLogo(150.dp, awake = true, animated = false)
                    com.somnia.app.ui.components.PoliceLogo(150.dp, awake = false, animated = false)
                }
            }
        }
    }

    @Test fun tutoHello() = shot { com.somnia.app.ui.screens.OnboardingFrame(0, 1f) }

    @Test fun splash() = paparazzi.snapshot { SomniaTheme { com.somnia.app.ui.screens.SplashPoster(onDone = {}, autoCloseMs = 999_999) } }

    @Test fun liveSceneMockup() = paparazzi.snapshot {
        SomniaTheme {
            androidx.compose.foundation.layout.Box {
                com.somnia.app.ui.components.NightScene()
                TonightLive(
                    LiveState(
                        sessionId = 1, startTime = t0, levelDb = -42f, noiseFloorDb = -55f, thresholdDb = -44f, capturing = false,
                        eventCount = 9, counts = mapOf(SoundType.SNORING to 5, SoundType.SPEECH to 2, SoundType.LAUGHTER to 1, SoundType.CRYING to 1),
                        lastEvent = SoundType.SPEECH, lastEventTime = t0 + 3_000_000L, alarmTarget = t0 + 30_000_000L,
                    ),
                    now = t0 + 2 * 3_600_000L + 14 * 60_000L, topInset = 150.dp,
                ) {}
            }
        }
    }

    private fun idleAt(h: Int) = shot {
        TonightIdle(
            settings = Settings(smartAlarmEnabled = true, alarmHour = 7, alarmMinute = 15),
            lastNight = null, tags = emptySet(), onToggleTag = {}, onStart = {}, onOpen = {}, hour = h,
        )
    }
    @Test fun bubble01h() = idleAt(1)
    @Test fun bubble03h() = idleAt(3)
    @Test fun bubble09h() = idleAt(9)

    @Test fun agent2() = paparazzi.snapshot {
        SomniaTheme {
            androidx.compose.runtime.CompositionLocalProvider(LocalAgent provides 1) {
                NightBackground {
                    TonightIdle(
                        settings = Settings(), lastNight = null, tags = emptySet(), onToggleTag = {}, onStart = {}, onOpen = {}, hour = 15,
                    )
                }
            }
        }
    }
    @Test fun agent2Moods() = paparazzi.snapshot {
        SomniaTheme {
            NightBackground {
                androidx.compose.foundation.layout.Column(androidx.compose.ui.Modifier.padding(16.dp)) {
                    androidx.compose.foundation.layout.Row {
                        for (h in listOf(15, 0, 3)) androidx.compose.runtime.CompositionLocalProvider(LocalAgent provides 1, LocalHour provides h) {
                            com.somnia.app.ui.components.PoliceLogoFrame(120.dp, 1f)
                        }
                    }
                    androidx.compose.foundation.layout.Row {
                        for (f in listOf(0, 1, 2)) androidx.compose.runtime.CompositionLocalProvider(LocalAgent provides 1) {
                            com.somnia.app.ui.components.PoliceLogoFrame(120.dp, 0f, 0.4f, sleepFrame = f)
                        }
                    }
                }
            }
        }
    }

    @Test fun waveform() = paparazzi.snapshot {
        val f = java.io.File.createTempFile("wave", ".wav")
        val pcm = ShortArray(16000 * 6) { i ->
            val t = i / 16000.0
            val env = if (t in 2.0..3.2) 0.6 else if (t in 4.5..4.8) 0.9 else 0.05
            (kotlin.math.sin(i * 0.07) * env * 20000 * (0.6 + 0.4 * kotlin.math.sin(t * 9))).toInt().toShort()
        }
        com.somnia.app.audio.WavFile.write(f, pcm)
        val peaks = com.somnia.app.ui.components.wavePeaks(f.absolutePath)
        SomniaTheme {
            NightBackground {
                androidx.compose.foundation.layout.Column(androidx.compose.ui.Modifier.padding(20.dp)) {
                    com.somnia.app.ui.components.GlassCard(title = "Paroles · 02:14", icon = "🗣️") {
                        androidx.compose.foundation.Canvas(androidx.compose.ui.Modifier.fillMaxWidth().height(64.dp)) {
                            val n = peaks.size; val slot = size.width / n
                            for (i in 0 until n) {
                                val h = peaks[i] * size.height * 0.92f
                                drawRoundRect(
                                    if (i < n * 0.45f) androidx.compose.ui.graphics.Color(0xFF8DB8E0) else androidx.compose.ui.graphics.Color(0x528DB8E0),
                                    androidx.compose.ui.geometry.Offset(i * slot, size.height / 2 - h / 2),
                                    androidx.compose.ui.geometry.Size(slot * 0.62f, h),
                                    androidx.compose.ui.geometry.CornerRadius(slot * 0.31f),
                                )
                            }
                        }
                    }
                }
            }
        }
    }

    @Test fun widget() {
        val v = paparazzi.inflate<android.view.View>(com.somnia.app.R.layout.widget_dodo)
        v.layoutParams = android.view.ViewGroup.LayoutParams((280 * 2.625f).toInt(), (120 * 2.625f).toInt())
        val frame = android.widget.FrameLayout(paparazzi.context).apply {
            setBackgroundColor(0xFF5A6C8C.toInt()); setPadding(40, 40, 40, 40); addView(v)
        }
        paparazzi.snapshot(frame)
    }
}
