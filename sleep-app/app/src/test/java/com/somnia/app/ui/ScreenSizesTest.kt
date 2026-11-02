package com.somnia.app.ui

import app.cash.paparazzi.DeviceConfig
import app.cash.paparazzi.Paparazzi
import com.somnia.app.data.Settings
import com.somnia.app.ui.components.NightBackground
import com.somnia.app.ui.screens.TonightIdle
import com.somnia.app.ui.theme.SomniaTheme
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.widthIn
import androidx.compose.ui.unit.dp
import org.junit.Rule
import org.junit.Test
import org.junit.runner.RunWith
import org.junit.runners.Parameterized

/** L'écran d'accueil rendu sur un petit, un moyen, un grand téléphone et une tablette. */
@RunWith(Parameterized::class)
class ScreenSizesTest(private val name: String, device: DeviceConfig) {
    companion object {
        @JvmStatic
        @Parameterized.Parameters(name = "{0}")
        fun devices() = listOf(
            arrayOf("petit", DeviceConfig.NEXUS_4),
            arrayOf("moyen", DeviceConfig.PIXEL_5),
            arrayOf("grand", DeviceConfig.PIXEL_6_PRO),
            arrayOf("tablette", DeviceConfig.NEXUS_10),
        )
    }

    @get:Rule
    val paparazzi = Paparazzi(deviceConfig = device, theme = "android:Theme.Material.NoActionBar", maxPercentDifference = 1.0)

    @Test fun home() = paparazzi.snapshot(name) {
        SomniaTheme {
            NightBackground {
                androidx.compose.foundation.layout.Box(
                    androidx.compose.ui.Modifier.fillMaxSize(), contentAlignment = androidx.compose.ui.Alignment.TopCenter,
                ) {
                    androidx.compose.foundation.layout.Box(androidx.compose.ui.Modifier.widthIn(max = 600.dp)) {
                        TonightIdle(Settings(), null, emptySet(), {}, {}, {}, hour = 22)
                    }
                }
            }
        }
    }
}
