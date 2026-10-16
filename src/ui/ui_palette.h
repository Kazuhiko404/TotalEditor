#pragma once

#include <mygui/MyGUI_Colour.h>

namespace totaleditor
{
// High-contrast, dark-lacquer and old-gold/amber design system shared with TotalSparring.
// Formulated to survive MyGUI skin texture multiplication (including dark UI mods like DarkUI).

const MyGUI::Colour kLacquer(20.f / 255.f, 17.f / 255.f, 16.f / 255.f, 1.f);
const MyGUI::Colour kBgDark(30.f / 255.f, 27.f / 255.f, 24.f / 255.f, 1.f);
const MyGUI::Colour kTextPrimary(238.f / 255.f, 228.f / 255.f, 210.f / 255.f, 1.f);

// Old gold, for headings and the accents that frame them.
const MyGUI::Colour kBrass(214.f / 255.f, 176.f / 255.f, 116.f / 255.f, 1.f);
const MyGUI::Colour kGoldDim(150.f / 255.f, 120.f / 255.f, 78.f / 255.f, 1.f);
const MyGUI::Colour kMuted(162.f / 255.f, 150.f / 255.f, 132.f / 255.f, 1.f);

// Amber accent for active states and primary actions.
const MyGUI::Colour kAmber(236.f / 255.f, 184.f / 255.f, 96.f / 255.f, 1.f);

// Vermilion / red accent for danger, errors, and destructive actions.
const MyGUI::Colour kDanger(206.f / 255.f, 74.f / 255.f, 58.f / 255.f, 1.f);

// Ready / success green accent.
const MyGUI::Colour kReady(124.f / 255.f, 172.f / 255.f, 116.f / 255.f, 1.f);

// Subtle dividers and borders.
const MyGUI::Colour kLine(58.f / 255.f, 51.f / 255.f, 44.f / 255.f, 1.f);
const MyGUI::Colour kSumi(42.f / 255.f, 37.f / 255.f, 33.f / 255.f, 1.f);
} // namespace totaleditor
