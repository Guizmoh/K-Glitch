package com.somnia.app.ui.theme

import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.darkColorScheme
import androidx.compose.runtime.Composable
import androidx.compose.ui.graphics.Color
import com.somnia.app.audio.SoundType

val Night = Color(0xFF0E1230)
val NightCard = Color(0xFF1A1F45)
val Moon = Color(0xFFB9A7FF)
val Dawn = Color(0xFFFFC38A)
val DeepColor = Color(0xFF5B5BD6)
val LightColor = Color(0xFF8FB7FF)
val AwakeColor = Color(0xFFFF9F7A)

fun SoundType.color(): Color = when (this) {
    SoundType.SPEECH -> Color(0xFF7FD1FF)
    SoundType.NIGHT_TERROR -> Color(0xFFFF6B6B)
    SoundType.SNORING -> Color(0xFFB9A7FF)
    SoundType.LAUGHTER -> Color(0xFFFFD166)
    SoundType.CRYING -> Color(0xFF64B5F6)
    SoundType.COUGH -> Color(0xFF9BE89B)
    SoundType.BREATHING -> Color(0xFF80CBC4)
    SoundType.NOISE -> Color(0xFF9E9EB8)
}

private val scheme = darkColorScheme(
    primary = Moon,
    onPrimary = Night,
    secondary = Dawn,
    background = Night,
    surface = Night,
    surfaceVariant = NightCard,
    surfaceContainer = NightCard,
    surfaceContainerHigh = NightCard,
    onBackground = Color(0xFFE8E6FF),
    onSurface = Color(0xFFE8E6FF),
    onSurfaceVariant = Color(0xFFB8B5D8),
)

@Composable
fun SomniaTheme(content: @Composable () -> Unit) = MaterialTheme(colorScheme = scheme, content = content)
