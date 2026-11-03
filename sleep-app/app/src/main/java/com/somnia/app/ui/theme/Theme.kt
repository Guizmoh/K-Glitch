package com.somnia.app.ui.theme

import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.rounded.*
import androidx.compose.material3.LocalContentColor
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Shapes
import androidx.compose.material3.Typography
import androidx.compose.runtime.Composable
import androidx.compose.runtime.CompositionLocalProvider
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.TextStyle
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.somnia.app.audio.SoundType

/** Police ronde et chaleureuse (Fredoka, licence OFL) pour le titre et le gros bouton. */
val CozyFont = androidx.compose.ui.text.font.FontFamily(
    androidx.compose.ui.text.font.Font(com.somnia.app.R.font.fredoka_semibold, FontWeight.SemiBold),
    androidx.compose.ui.text.font.Font(com.somnia.app.R.font.fredoka_bold, FontWeight.Bold),
)

/** Police du gros bouton (Quicksand, licence OFL), différente de celle du titre. */
val ButtonFont = androidx.compose.ui.text.font.FontFamily(
    androidx.compose.ui.text.font.Font(com.somnia.app.R.font.quicksand_bold, FontWeight.Bold),
)

/** Police douce et très lisible (Nunito, licence OFL) pour l'écran de nuit. */
val NightFont = androidx.compose.ui.text.font.FontFamily(
    androidx.compose.ui.text.font.Font(com.somnia.app.R.font.nunito_semibold, FontWeight.SemiBold),
    androidx.compose.ui.text.font.Font(com.somnia.app.R.font.nunito_extrabold, FontWeight.ExtraBold),
)

// Palette « Police du Dodo » : bleu nuit, bleu police, or de l'insigne.
// Couleurs du thème actif (voir AppThemes.kt) : lues à chaque affichage.
val Night get() = Themes.cur.bgBottom
val NightMid get() = Themes.cur.bgMid
val NightCard get() = Themes.cur.card // vitre teintée semi-opaque
val CardStroke get() = Themes.cur.cardStroke
val PoliceBlue get() = Themes.cur.accent
val PoliceBlueDark get() = Themes.cur.accent.copy(alpha = 0.22f)
val Badge get() = Themes.cur.accent
val Moon get() = Themes.cur.accent2
val TextSoft get() = Themes.cur.textSoft
val DeepColor get() = Themes.cur.deep
val LightColor get() = Themes.cur.light
val AwakeColor get() = Themes.cur.awake
val Good get() = Themes.cur.good

val BackgroundBrush get() = Brush.verticalGradient(listOf(Themes.cur.bgTop, Themes.cur.bgMid, Themes.cur.bgBottom))
val PrimaryBrush get() = Brush.linearGradient(listOf(Themes.cur.accent, Themes.cur.accent))
val SunriseBrush = Brush.linearGradient(listOf(Color(0xFFF1E6CF), Color(0xFFF1E6CF)))

/** Sur fond clair, les couleurs pastel des sons sont assombries pour rester lisibles. */
private fun Color.forTheme(): Color =
    if (Themes.cur.dark) this else androidx.compose.ui.graphics.lerp(this, Color(0xFF1B1840), 0.42f)

fun SoundType.color(): Color = baseColor().forTheme()

private fun SoundType.baseColor(): Color = when (this) {
    SoundType.SPEECH -> Color(0xFF8DB8E0)
    SoundType.NIGHT_TERROR -> Color(0xFFE08A7E)
    SoundType.SNORING -> Color(0xFFB3A6D9)
    SoundType.LAUGHTER -> Color(0xFFE6C47A)
    SoundType.CRYING -> Color(0xFF7FA7D6)
    SoundType.COUGH -> Color(0xFF9CC9A3)
    SoundType.BREATHING -> Color(0xFF8CC2BC)
    SoundType.NOISE -> Color(0xFF9AA2C0)
    SoundType.MEDIA -> Color(0xFFA9B6D6)
}

fun scoreColor(score: Int?): Color = when {
    score == null -> Color(0xFF9E9EB8)
    score >= 80 -> Good
    score >= 60 -> Badge
    else -> AwakeColor
}

private fun schemeFor(t: AppTheme) = (if (t.dark) androidx.compose.material3.darkColorScheme() else androidx.compose.material3.lightColorScheme()).copy(
    primary = t.accent, onPrimary = t.onAccent,
    primaryContainer = t.accent.copy(alpha = 0.22f), onPrimaryContainer = t.text,
    secondary = t.accent, onSecondary = t.onAccent,
    secondaryContainer = t.accent.copy(alpha = 0.2f), onSecondaryContainer = t.accent,
    background = t.bgBottom, surface = t.bgBottom, surfaceVariant = t.card,
    surfaceContainer = t.bgMid, surfaceContainerHigh = t.bgMid, surfaceContainerHighest = t.bgTop,
    onBackground = t.text, onSurface = t.text, onSurfaceVariant = t.textSoft,
    outline = t.cardStroke, outlineVariant = t.cardStroke,
    error = if (t.dark) Color(0xFFE08A7E) else Color(0xFFB3412F),
)

private val typography = Typography().let { t ->
    t.copy(
        // Titres à empattements, comme le titre de l'affiche
        displaySmall = t.displaySmall.copy(fontFamily = FontFamily.Serif, fontWeight = FontWeight.SemiBold),
        headlineLarge = t.headlineLarge.copy(fontFamily = FontFamily.Serif, fontWeight = FontWeight.SemiBold),
        headlineMedium = t.headlineMedium.copy(fontFamily = FontFamily.Serif, fontWeight = FontWeight.SemiBold),
        headlineSmall = t.headlineSmall.copy(fontFamily = FontFamily.Serif, fontWeight = FontWeight.SemiBold),
        titleLarge = t.titleLarge.copy(fontWeight = FontWeight.SemiBold),
        titleMedium = t.titleMedium.copy(fontWeight = FontWeight.Medium),
        // Petits textes agrandis et en semi-gras (lisibilité de près)
        bodyMedium = t.bodyMedium.copy(fontSize = 15.sp, fontWeight = FontWeight.Medium),
        bodySmall = t.bodySmall.copy(fontSize = 14.sp, lineHeight = 19.sp, fontWeight = FontWeight.SemiBold),
        labelLarge = t.labelLarge.copy(fontSize = 15.sp, fontWeight = FontWeight.SemiBold),
        labelMedium = t.labelMedium.copy(fontSize = 14.sp, fontWeight = FontWeight.SemiBold),
        labelSmall = TextStyle(fontSize = 13.sp, lineHeight = 17.sp, letterSpacing = 0.2.sp, fontWeight = FontWeight.SemiBold),
    )
}

private val shapes = Shapes(
    small = RoundedCornerShape(12.dp),
    medium = RoundedCornerShape(20.dp),
    large = RoundedCornerShape(28.dp),
)

@Composable
fun SomniaTheme(forceNight: Boolean = false, content: @Composable () -> Unit) {
    if (forceNight) Themes.nightOverride = true
    val t = Themes.cur
    // Adaptation à la taille de l'écran : l'interface est dessinée pour un écran de
    // référence (411 x 860 dp, type Pixel 6) puis agrandie ou réduite à l'identique.
    val d = androidx.compose.ui.platform.LocalDensity.current
    val conf = androidx.compose.ui.platform.LocalConfiguration.current
    val scale = uiScale(conf.screenWidthDp, conf.screenHeightDp)
    MaterialTheme(colorScheme = schemeFor(t), typography = typography, shapes = shapes) {
        CompositionLocalProvider(
            LocalContentColor provides t.text,
            // Texte agrandi d'environ 12 % (en plus du réglage de taille du téléphone).
            androidx.compose.ui.platform.LocalDensity provides androidx.compose.ui.unit.Density(d.density * scale, d.fontScale * 1.12f),
            content = content,
        )
    }
}

/**
 * Écrans de nuit (surveillance, réveil) : toujours le thème sombre, devant le décor de nuit,
 * quel que soit le thème choisi.
 */
@Composable
fun ForceNight(content: @Composable () -> Unit) {
    androidx.compose.runtime.remember { Themes.nightOverride = true }
    androidx.compose.runtime.DisposableEffect(Unit) { onDispose { Themes.nightOverride = false } }
    MaterialTheme(colorScheme = schemeFor(Themes.BLEU), typography = MaterialTheme.typography, shapes = MaterialTheme.shapes) {
        CompositionLocalProvider(LocalContentColor provides Themes.BLEU.text, content = content)
    }
}

/**
 * Facteur d'échelle de l'interface selon la taille de l'écran (en dp réels).
 * Base 411 dp de large ; on tient aussi compte des écrans très courts.
 * Borné entre 0,78 (très petits téléphones) et 1,30 (grands écrans, tablettes).
 */
fun uiScale(widthDp: Int, heightDp: Int): Float {
    if (widthDp <= 0 || heightDp <= 0) return 1f
    val portraitW = minOf(widthDp, heightDp)
    val portraitH = maxOf(widthDp, heightDp)
    val byWidth = portraitW / 411f
    val byHeight = portraitH / 780f
    return minOf(byWidth, byHeight).coerceIn(0.78f, 1.30f)
}

fun SoundType.icon(): androidx.compose.ui.graphics.vector.ImageVector = when (this) {
    SoundType.SPEECH -> Icons.Rounded.RecordVoiceOver
    SoundType.NIGHT_TERROR -> Icons.Rounded.Campaign
    SoundType.SNORING -> Icons.Rounded.Waves
    SoundType.LAUGHTER -> Icons.Rounded.SentimentVerySatisfied
    SoundType.CRYING -> Icons.Rounded.SentimentVeryDissatisfied
    SoundType.COUGH -> Icons.Rounded.Sick
    SoundType.BREATHING -> Icons.Rounded.Air
    SoundType.NOISE -> Icons.Rounded.GraphicEq
    SoundType.MEDIA -> Icons.Rounded.Tv
}
