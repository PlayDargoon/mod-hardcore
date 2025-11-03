/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license: https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3
 */

// From SC
void AddSC_mod_hardcore();
void AddSC_hardcore_spell_scripts();

// Add all scripts - using both names for compatibility
void Addmod_hardcoreScripts()
{
    AddSC_mod_hardcore();
    AddSC_hardcore_spell_scripts();
}

// Alternative name for auto-generated loader
void AddHardcoreScripts()
{
    AddSC_mod_hardcore();
    AddSC_hardcore_spell_scripts();
}
