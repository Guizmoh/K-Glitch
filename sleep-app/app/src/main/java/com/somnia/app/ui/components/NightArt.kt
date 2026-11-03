package com.somnia.app.ui.components

import androidx.compose.foundation.Image
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.res.painterResource
import com.somnia.app.R

/** Fond illustré de l'écran de nuit : ciel, lune, réverbère, ville, rebord en bois. */
@Composable
fun NightBackdrop(modifier: Modifier = Modifier.fillMaxSize()) {
    Image(painterResource(R.drawable.night_bg), null, modifier, contentScale = ContentScale.Crop, alignment = Alignment.TopCenter)
}

/** Feuillages au premier plan, par-dessus l'interface (ne bloquent pas les doigts). */
@Composable
fun NightFoliage(modifier: Modifier = Modifier.fillMaxSize()) {
    Image(painterResource(R.drawable.night_foliage), null, modifier, contentScale = ContentScale.FillBounds)
}
