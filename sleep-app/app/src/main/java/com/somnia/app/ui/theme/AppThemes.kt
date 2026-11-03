package com.somnia.app.ui.theme

import android.content.Context
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import androidx.compose.ui.graphics.Color

/** Couleurs d'un thème de l'application. */
class AppTheme(
    val id: String,
    val name: String,
    val dark: Boolean,
    val bgTop: Color, val bgMid: Color, val bgBottom: Color,
    val card: Color, val cardStroke: Color,
    val text: Color, val textSoft: Color,
    val accent: Color, val onAccent: Color, val accent2: Color,
    val good: Color, val awake: Color, val deep: Color, val light: Color,
    val buttonTop: Color, val buttonBottom: Color, val buttonEdge: Color, val buttonText: Color,
    val bubble: Color, val bubbleText: Color,
    val nav: Color,
)

object Themes {
    /** Bleu nuit : le thème d'origine (sombre). */
    val BLEU = AppTheme(
        "bleu", "Bleu nuit", dark = true,
        bgTop = Color(0xFF34509A), bgMid = Color(0xFF283E80), bgBottom = Color(0xFF213268),
        card = Color(0x5A16234F), cardStroke = Color(0x3DFFFFFF),
        text = Color(0xFFEDE8DC), textSoft = Color(0xFFB9C1DE),
        accent = Color(0xFFE6C47A), onAccent = Color(0xFF151A33), accent2 = Color(0xFFC9BCFF),
        good = Color(0xFF8FCFA5), awake = Color(0xFFE3A07F), deep = Color(0xFF4A5AA8), light = Color(0xFF8FB7FF),
        buttonTop = Color(0xFFB9DDF8), buttonBottom = Color(0xFF86B8EC), buttonEdge = Color(0xFF4C79BF), buttonText = Color(0xFF13305F),
        bubble = Color(0xA8D9E4FA), bubbleText = Color(0xFF14204A),
        nav = Color(0xE01C2A5E),
    )

    /** Violet : dégradé pastel lavande → rose (clair). Thème de base. */
    val VIOLET = AppTheme(
        "violet", "Violet", dark = false,
        bgTop = Color(0xFFD3D8F5), bgMid = Color(0xFFE0D2F1), bgBottom = Color(0xFFF3CFE9),
        card = Color(0x80FFFFFF), cardStroke = Color(0xB3FFFFFF),
        text = Color(0xFF2C2556), textSoft = Color(0xFF5D5885),
        accent = Color(0xFF7B52C9), onAccent = Color.White, accent2 = Color(0xFF9A7BE0),
        good = Color(0xFF2E8F5B), awake = Color(0xFFC9603A), deep = Color(0xFF5B63C9), light = Color(0xFF6F97E0),
        buttonTop = Color(0xFFCBCEFB), buttonBottom = Color(0xFF9EA4EE), buttonEdge = Color(0xFF6F76C8), buttonText = Color(0xFF241E5C),
        bubble = Color(0xB3FFFFFF), bubbleText = Color(0xFF2C2556),
        nav = Color(0xE6EDE6F8),
    )

    /** Rose pâle : dégradé du blanc rosé au rose (clair). */
    val ROSE = AppTheme(
        "rose", "Rose pâle", dark = false,
        bgTop = Color(0xFFFDF1F7), bgMid = Color(0xFFF9D5E6), bgBottom = Color(0xFFF1A9CB),
        card = Color(0x80FFFFFF), cardStroke = Color(0xB3FFFFFF),
        text = Color(0xFF4A1F3D), textSoft = Color(0xFF7E4C69),
        accent = Color(0xFFC2417F), onAccent = Color.White, accent2 = Color(0xFFD36FA6),
        good = Color(0xFF2E8F5B), awake = Color(0xFFC9603A), deep = Color(0xFF9C4E9C), light = Color(0xFFD9779F),
        buttonTop = Color(0xFFFAC9E1), buttonBottom = Color(0xFFEE97C3), buttonEdge = Color(0xFFC25C93), buttonText = Color(0xFF4A1F3D),
        bubble = Color(0xB3FFFFFF), bubbleText = Color(0xFF4A1F3D),
        nav = Color(0xE6FBE3EF),
    )

    /** Dans l'ordre d'affichage (le thème de base d'abord). */
    val ALL = listOf(VIOLET, ROSE, BLEU)

    private var selected by mutableStateOf(VIOLET)
    /** Vrai sur les écrans de nuit (surveillance, réveil) : toujours le thème sombre. */
    var nightOverride by mutableStateOf(false)

    val cur: AppTheme get() = if (nightOverride) BLEU else selected
    val chosen: AppTheme get() = selected

    fun init(ctx: Context) {
        val id = ctx.getSharedPreferences("app", Context.MODE_PRIVATE).getString("theme", null)
        selected = ALL.firstOrNull { it.id == id } ?: VIOLET
    }

    /** Change de thème sans le mémoriser (aperçus, tests). */
    fun select(theme: AppTheme) { selected = theme }

    fun set(ctx: Context, theme: AppTheme) {
        selected = theme
        ctx.getSharedPreferences("app", Context.MODE_PRIVATE).edit().putString("theme", theme.id).apply()
    }
}
