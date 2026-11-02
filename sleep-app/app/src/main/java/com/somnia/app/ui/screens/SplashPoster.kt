package com.somnia.app.ui.screens

import androidx.compose.animation.core.Animatable
import androidx.compose.animation.core.LinearEasing
import androidx.compose.animation.core.tween
import androidx.compose.foundation.Image
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.interaction.MutableInteractionSource
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.navigationBarsPadding
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.remember
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.alpha
import androidx.compose.ui.draw.scale
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.unit.dp
import com.somnia.app.R
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch

/** Écran d'accueil : l'affiche « Police du Dodo » avec un lent zoom, puis l'appli. */
@Composable
fun SplashPoster(onDone: () -> Unit, autoCloseMs: Long = 3200) {
    val zoom = remember { Animatable(1f) }
    val hint = remember { Animatable(0f) }
    LaunchedEffect(Unit) {
        launch { zoom.animateTo(1.08f, tween(autoCloseMs.toInt() + 800, easing = LinearEasing)) }
        launch { delay(1200); hint.animateTo(1f, tween(600)) }
        delay(autoCloseMs)
        onDone()
    }
    Box(
        Modifier.fillMaxSize().background(Color(0xFF0B1026))
            .clickable(remember { MutableInteractionSource() }, indication = null) { onDone() },
    ) {
        Image(
            painterResource(R.drawable.splash_poster), "Police du Dodo",
            Modifier.fillMaxSize().scale(zoom.value), contentScale = ContentScale.Crop,
        )
        Text(
            "Touchez pour entrer",
            Modifier.align(Alignment.BottomCenter).navigationBarsPadding().padding(bottom = 28.dp).alpha(hint.value * 0.85f),
            color = Color(0xFFFFF3D6),
        )
    }
}
