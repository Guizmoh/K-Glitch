package com.somnia.app.ui.components

// Généré par tools/logo.py : ne pas modifier à la main.

import androidx.compose.ui.graphics.Color

/** Formes du logo « Police du Dodo » (repère 108 x 108). */
internal object LogoArt {
    class Grad(val x1: Float, val y1: Float, val x2: Float, val y2: Float, val from: Color, val to: Color)
    class Layer(val path: String, val fill: Color? = null, val grad: Grad? = null, val stroke: Color? = null, val width: Float = 0f, val dy: Float = 0f)

    private val G_UNIFORM = Grad(54f, 80f, 54f, 108f, Color(0xFF26337F), Color(0xFF121845))
    private val G_SKIN = Grad(54f, 40f, 54f, 76f, Color(0xFFFBDDC4), Color(0xFFF0C29E))
    private val G_CROWN = Grad(54f, 24f, 54f, 47f, Color(0xFF34459F), Color(0xFF161E58))
    private val G_VISOR = Grad(54f, 48f, 54f, 56f, Color(0xFF262C52), Color(0xFF05070F))
    private val G_GOLD = Grad(54f, 31f, 54f, 44f, Color(0xFFF8DC8A), Color(0xFFC48F2A))

    val layers = listOf(
        Layer("M16,108 L16,101 Q18,87 37,82.5 L46,80 L54,90 L62,80 L71,82.5 Q90,87 92,101 L92,108 Z", grad = G_UNIFORM), // uniform
        Layer("M46,80 L54,90 L62,80 L60.5,76 L47.5,76 Z", fill = Color(0xFFD6E4F7)), // shirt
        Layer("M47.5,70 L60.5,70 L60.5,78.5 Q54,82.5 47.5,78.5 Z", fill = Color(0xFFE3B48F)), // neck
        Layer("M51.6,80.6 L56.4,80.6 L55.5,84.6 L52.5,84.6 Z", fill = Color(0xFF0D1230)), // tie_knot
        Layer("M52.4,84.4 L55.6,84.4 L57,96 L54,99.5 L51,96 Z", fill = Color(0xFF0D1230)), // tie
        Layer("M41.5,87m-1.4,0a1.4,1.4 0,1 1,2.8 0a1.4,1.4 0,1 1,-2.8 0", grad = G_GOLD), // collar_pin_l
        Layer("M66.5,87m-1.4,0a1.4,1.4 0,1 1,2.8 0a1.4,1.4 0,1 1,-2.8 0", grad = G_GOLD), // collar_pin_r
        Layer("M38.5,58m-3,0a3,4.5 0,1 1,6 0a3,4.5 0,1 1,-6 0", fill = Color(0xFFEDBD98)), // ear_l
        Layer("M69.5,58m-3,0a3,4.5 0,1 1,6 0a3,4.5 0,1 1,-6 0", fill = Color(0xFFEDBD98)), // ear_r
        Layer("M39,52 Q39,74.5 54,75.5 Q69,74.5 69,52 Q69,40 54,40 Q39,40 39,52 Z", grad = G_SKIN), // face
        Layer("M39.2,49 L42.4,49 L42.4,57.5 Q40.2,55.8 39.3,53.5 Z", fill = Color(0xFF4A3426)), // hair_l
        Layer("M68.8,49 L65.6,49 L65.6,57.5 Q67.8,55.8 68.7,53.5 Z", fill = Color(0xFF4A3426)), // hair_r
        Layer("M44.5,65.2m-2.6,0a2.6,1.6 0,1 1,5.2 0a2.6,1.6 0,1 1,-5.2 0", fill = Color(0x4DFF9E8C)), // cheek_l
        Layer("M63.5,65.2m-2.6,0a2.6,1.6 0,1 1,5.2 0a2.6,1.6 0,1 1,-5.2 0", fill = Color(0x4DFF9E8C)), // cheek_r
        Layer("M44.6,55 Q47.5,53.4 50.4,54.8", stroke = Color(0xFF4A3426), width = 1.6f), // brow_l
        Layer("M57.6,54.8 Q60.5,53.4 63.4,55", stroke = Color(0xFF4A3426), width = 1.6f), // brow_r
        Layer("M54.3,59.6 Q52.6,63.6 54.8,64.3", stroke = Color(0xFFD49A76), width = 1.2f), // nose
        Layer("M54,66 Q50.2,64 46.6,66.5 Q49.5,68 54,67.1 Q58.5,68 61.4,66.5 Q57.8,64 54,66 Z", fill = Color(0xFF4A3426)), // mustache
        Layer("M51.9,70.1 Q54,71.4 56.1,70.1", stroke = Color(0xFFB5776A), width = 1.1f), // mouth
        Layer("M31,38 Q32,27 54,24 Q76,27 77,38 Q74.5,43.5 70,46.5 L38,46.5 Q33.5,43.5 31,38 Z", grad = G_CROWN, dy = -3.5f), // crown
        Layer("M37,31.5 Q54,25.8 71,31.5 Q54,28.6 37,31.5 Z", fill = Color(0x26FFFFFF), dy = -3.5f), // crown_shine
        Layer("M38,45 L70,45 L70,50.6 L38,50.6 Z", fill = Color(0xFF0D1230), dy = -3.5f), // band
        Layer("M38.5,46.4 L69.5,46.4", stroke = Color(0xFFE0B656), width = 0.9f, dy = -3.5f), // band_cord
        Layer("M36.5,50 Q54,47.4 71.5,50 Q72,53.6 66,55.2 Q54,57.6 42,55.2 Q36,53.6 36.5,50 Z", grad = G_VISOR, dy = -3.5f), // visor
        Layer("M42.5,51.6 Q54,50 65.5,51.6", stroke = Color(0x40FFFFFF), width = 0.7f, dy = -3.5f), // visor_shine
        Layer("M54,30.8 L60.2,33 Q60.2,40.6 54,43.8 Q47.8,40.6 47.8,33 Z", grad = G_GOLD, dy = -3.5f), // badge
        Layer("M55.3,34.4 a3,3 0,1 0,0.9 5.6 a2.3,2.3 0,1 1,-0.9 -5.6 z", fill = Color(0xFF1C2766), dy = -3.5f), // badge_moon
    )

    val eyeCenters = listOf(47.5f to 58.6f, 60.5f to 58.6f)
    const val EYE_RX = 2.7f
    const val EYE_RY = 2.3f
    const val IRIS_R = 1.65f
    val eyeWhite = Color(0xFFFFFFFF)
    val iris = Color(0xFF27304F)
    val lid = Color(0xFF4A3426)
    val zzz = listOf("M71,21 L75.5,21 L71,25.5 L75.5,25.5", "M77.5,13.5 L80.8,13.5 L77.5,16.8 L80.8,16.8")
}
