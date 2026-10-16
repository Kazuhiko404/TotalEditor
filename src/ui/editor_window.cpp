#include "common.h"
#include "editor_window.h"
#include "anatomy.h"
#include "catalog.h"
#include "config.h"
#include "log.h"
#include "messages.h"
#include "appearance.h"
#include "personality.h"
#include "race_edit.h"
#include "store.h"
#include "ui_widgets.h"
#include "ui_palette.h"
#include "character_identity.h"
#include "colour_adjust_rules.h"
#include "portrait_editor.h"
#include "uniform_colour.h"
#include "uniform_engine.h"
#include "uniform_store.h"

#include <mygui/MyGUI.h>

namespace totaleditor
{
namespace
{
// Distinct names so these widgets coexist with other mods' GUI.
const char* const kRootName       = "TE_Root";
const char* const kWarningName    = "TE_Warning";
const char* const kRaceComboName  = "TE_RaceCombo";
const char* const kBonusListName  = "TE_BonusList";
const char* const kBonusStatName  = "TE_BonusStat";
const char* const kBonusValueName = "TE_BonusValue";
const char* const kBonusBarName   = "TE_BonusBar";
const char* const kBonusAddName   = "TE_BonusAdd";
const char* const kBonusDelName   = "TE_BonusDel";
const char* const kRevertName     = "TE_Revert";
const char* const kSubjectName    = "TE_Subject";
const char* const kStatComboName  = "TE_StatCombo";
const char* const kStatValueName  = "TE_StatValue";
const char* const kStatBarName    = "TE_StatBar";
const char* const kStatApplyName  = "TE_StatApply";
const char* const kStatusName     = "TE_Status";
const char* const kCopyComboName  = "TE_CopyCombo";
const char* const kCopyButtonName = "TE_CopyButton";
const char* const kBonusHeadName  = "TE_BonusHead";
const char* const kStatHeadName   = "TE_StatHead";
const char* const kCopyHeadName   = "TE_CopyHead";
const char* const kFilterName     = "TE_Filter";
const char* const kFilterGoName   = "TE_FilterGo";
const char* const kCopyTraitsName = "TE_CopyTraits";
const char* const kSubjectRaceName = "TE_SubjectRace";
const char* const kJumpRaceName    = "TE_JumpRace";
const char* const kEditAppearanceName = "TE_EditAppearance";
const char* const kEditHeadName    = "TE_EditHead";
const char* const kRaceLabelName   = "TE_RaceLabel";
const char* const kBonusLabelName  = "TE_BonusLabel";
const char* const kBonusSearchName = "TE_BonusSearch";
const char* const kBonusSearchGoName = "TE_BonusSearchGo";
const char* const kBonusValueLabelName = "TE_BonusValueLabel";
const char* const kCopyLabelName   = "TE_CopyLabel";
const char* const kCopySearchName  = "TE_CopySearch";
const char* const kCopySearchGoName = "TE_CopySearchGo";
const char* const kStatLabelName   = "TE_StatLabel";
const char* const kStatSearchName  = "TE_StatSearch";
const char* const kStatSearchGoName = "TE_StatSearchGo";
const char* const kStatValueLabelName = "TE_StatValueLabel";
const char* const kDetailsName     = "TE_Details";
const char* const kDetailsHeadName = "TE_DetailsHead";
const char* const kPartListName    = "TE_PartList";
const char* const kPartValueName   = "TE_PartValue";
const char* const kPartApplyName   = "TE_PartApply";
const char* const kPartHeadName    = "TE_PartHead";
const char* const kPartLabelName   = "TE_PartLabel";
const char* const kRealOnlyName    = "TE_RealOnly";
const char* const kHitValueName    = "TE_HitValue";
const char* const kHitLabelName    = "TE_HitLabel";
const char* const kHitApplyName    = "TE_HitApply";
const char* const kFlagListName    = "TE_FlagList";
const char* const kFlagToggleName  = "TE_FlagToggle";
const char* const kFlagHeadName    = "TE_FlagHead";
const char* const kTabsName        = "TE_Tabs";
const char* const kRealOnlyLabelName = "TE_RealOnlyLabel";
const char* const kCopyTraitsLabelName = "TE_CopyTraitsLabel";
const char* const kPageHelp1Name   = "TE_Help1";
const char* const kPageHelp2Name   = "TE_Help2";
const char* const kPageHelp3Name   = "TE_Help3";
const char* const kPageHelp4Name   = "TE_Help4";
const char* const kPageHelp5Name   = "TE_Help5";
const char* const kBonusListHeadName = "TE_BonusListHead";
const char* const kBloodHeadName   = "TE_BloodHead";
const char* const kBloodMinLabelName = "TE_BloodMinLabel";
const char* const kBloodMinName    = "TE_BloodMin";
const char* const kBloodMaxLabelName = "TE_BloodMaxLabel";
const char* const kBloodMaxName    = "TE_BloodMax";
const char* const kBloodApplyName  = "TE_BloodApply";
const char* const kSkillListName   = "TE_SkillList";
const char* const kSkillListHeadName = "TE_SkillListHead";
const char* const kRaceBonusListName = "TE_RaceBonusList";
const char* const kRaceBonusHeadName = "TE_RaceBonusHead";
const char* const kTargetDetailsName = "TE_TargetDetails";
const char* const kPersonalityHeadName    = "TE_PersonalityHead";
const char* const kPersonalityCurrentName = "TE_PersonalityCurrent";
const char* const kPersonalityComboName   = "TE_PersonalityCombo";
const char* const kPersonalityApplyName   = "TE_PersonalityApply";

// "Window", the layer KenshiLua's own windows use -- and the reason matters.
//
// A ComboBox drops its list onto the "Popup" layer, which MyGUI orders ABOVE
// "Window" but BELOW "Info". On "Info" the window painted over its own
// dropdowns: the list opened, drew underneath, and looked like nothing had
// happened. Sitting on "Window" puts the list back in front where it belongs.
const char* const kLayer = "Window";

const char* const kWindowSkins[] = { "Kenshi_WindowCX", "WindowCSX", "WindowC" };
const int kWindowSkinCount = 3;

// Sized for readability rather than compactness. The first cut used 26px rows
// and a three-line warning; in game the warning was clipped mid-sentence and
// the dropdowns were hard to hit, so every control grew and the window with it.
const int kMargin = 16;
const int kRow = 34;
const int kGap = 10;
const int kButtonHeight = 36;
const int kBarHeight = 26;

// A section heading above each group of controls, so the two panels are told
// apart at a glance rather than by reading their contents.
const int kHeadingHeight = 26;

// A label sitting directly above the control it names, and the tighter gap
// that binds the two together: a label separated by the usual gap reads as
// belonging to whatever is above it instead.
const int kLabelHeight = 20;
const int kTightGap = 4;

MyGUI::Window*   g_root = NULL;
MyGUI::Widget*   g_client = NULL;
MyGUI::EditBox*  g_warning = NULL;
MyGUI::ComboBox* g_raceCombo = NULL;
MyGUI::ComboBox* g_bonusStat = NULL;
MyGUI::EditBox*  g_bonusValue = NULL;
MyGUI::ScrollBar* g_bonusBar = NULL;
MyGUI::Button*   g_bonusAdd = NULL;
MyGUI::Button*   g_bonusDel = NULL;
MyGUI::Button*   g_revert = NULL;
MyGUI::TextBox*  g_subject = NULL;
MyGUI::ComboBox* g_statCombo = NULL;
MyGUI::EditBox*  g_statValue = NULL;
MyGUI::ScrollBar* g_statBar = NULL;
MyGUI::Button*   g_statApply = NULL;
MyGUI::TextBox*  g_status = NULL;
MyGUI::ComboBox* g_copyCombo = NULL;
MyGUI::Button*   g_copyButton = NULL;
MyGUI::TextBox*  g_bonusHead = NULL;
MyGUI::TextBox*  g_statHead = NULL;
MyGUI::TextBox*  g_copyHead = NULL;
MyGUI::EditBox*  g_filter = NULL;
MyGUI::Button*   g_filterGo = NULL;
MyGUI::Button*   g_copyTraits = NULL;
MyGUI::TextBox*  g_subjectRace = NULL;
MyGUI::Button*   g_jumpRace = NULL;
MyGUI::Button*   g_editAppearance = NULL;
MyGUI::TextBox*  g_editHead = NULL;
MyGUI::TextBox*  g_raceLabel = NULL;
MyGUI::TextBox*  g_bonusLabel = NULL;
MyGUI::EditBox*  g_bonusSearch = NULL;
MyGUI::Button*   g_bonusSearchGo = NULL;
MyGUI::TextBox*  g_bonusValueLabel = NULL;
MyGUI::TextBox*  g_copyLabel = NULL;
MyGUI::EditBox*  g_copySearch = NULL;
MyGUI::Button*   g_copySearchGo = NULL;
MyGUI::TextBox*  g_statLabel = NULL;
MyGUI::EditBox*  g_statSearch = NULL;
MyGUI::Button*   g_statSearchGo = NULL;
MyGUI::TextBox*  g_statValueLabel = NULL;
MyGUI::ListBox*  g_details = NULL;
MyGUI::TextBox*  g_detailsHead = NULL;
MyGUI::ListBox*  g_partList = NULL;
MyGUI::EditBox*  g_partValue = NULL;
MyGUI::Button*   g_partApply = NULL;
MyGUI::TextBox*  g_partHead = NULL;
MyGUI::TextBox*  g_partLabel = NULL;
MyGUI::Button*   g_realOnly = NULL;
MyGUI::EditBox*  g_hitValue = NULL;
MyGUI::TextBox*  g_hitLabel = NULL;
MyGUI::Button*   g_hitApply = NULL;
MyGUI::ListBox*  g_flagList = NULL;
MyGUI::Button*   g_flagToggle = NULL;
MyGUI::TextBox*  g_flagHead = NULL;
// The tab panel and its pages.
//
// Built through ForgottenGUI::createTabPanel -- the engine's own creator, the
// one KenshiLua exposes and the game uses for its own panels. Creating a
// MyGUI::TabControl directly with a guessed skin produced a control that
// linked, accepted the skin, and drew nothing at all.
//
// createTabPanel builds its own window at relative screen coordinates, so the
// panel IS the window here: there is no separate frame to parent it into.
const int kPageCount = 6;
MyGUI::Widget* g_pages[kPageCount] = { NULL };
MyGUI::Button* g_tabButtons[kPageCount] = { NULL };
OutlineButton  g_tabs[kPageCount];
MyGUI::delegates::IDelegate1<MyGUI::Widget*>* g_tabDelegates[kPageCount] = { NULL };
int g_currentPage = 0;

OutlineButton g_copyBtn;
OutlineButton g_revertBtn;
OutlineButton g_bonusAddBtn;
OutlineButton g_bonusDelBtn;
OutlineButton g_partApplyBtn;
OutlineButton g_hitApplyBtn;
OutlineButton g_bloodApplyBtn;
OutlineButton g_statApplyBtn;
OutlineButton g_personalityApplyBtn;
OutlineButton g_jumpRaceBtn;
OutlineButton g_editAppearanceBtn;
OutlineButton g_bonusSearchGoBtn;
OutlineButton g_copySearchGoBtn;
OutlineButton g_statSearchGoBtn;
OutlineButton g_filterGoBtn;
MyGUI::TextBox*  g_realOnlyLabel = NULL;
MyGUI::TextBox*  g_copyTraitsLabel = NULL;
MyGUI::EditBox*  g_pageHelp1 = NULL;
MyGUI::EditBox*  g_pageHelp2 = NULL;
MyGUI::EditBox*  g_pageHelp3 = NULL;
MyGUI::EditBox*  g_pageHelp4 = NULL;
MyGUI::EditBox*  g_pageHelp5 = NULL;
MyGUI::EditBox*  g_pageHelp6 = NULL;
MyGUI::TextBox*  g_bonusListHead = NULL;
MyGUI::TextBox*  g_bloodHead = NULL;
MyGUI::TextBox*  g_bloodMinLabel = NULL;
MyGUI::EditBox*  g_bloodMin = NULL;
MyGUI::TextBox*  g_bloodMaxLabel = NULL;
MyGUI::EditBox*  g_bloodMax = NULL;
MyGUI::Button*   g_bloodApply = NULL;
MyGUI::delegates::IDelegate1<MyGUI::Widget*>* g_bloodApplyDelegate = NULL;
MyGUI::ListBox*  g_skillList = NULL;
MyGUI::TextBox*  g_skillListHead = NULL;
MyGUI::delegates::IDelegate2<MyGUI::ListBox*, size_t>* g_skillListDelegate = NULL;

// The bonus page's own copy of the race's bonuses. Page one has a list of the
// same thing, but a widget belongs to one page, so the editing page carries its
// own -- read the sheet on the right, click a row, edit it on the left.
MyGUI::ListBox*  g_raceBonusList = NULL;
MyGUI::TextBox*  g_raceBonusHead = NULL;
MyGUI::delegates::IDelegate2<MyGUI::ListBox*, size_t>* g_raceBonusDelegate = NULL;

// What each row of that list stands for, so a click maps back to a bonus.
std::vector<BonusEntry> g_shownRaceBonuses;

// The same report as g_details, for the target race. Page one is where the two
// races are compared before a copy, so both sides show what they carry.
MyGUI::ListBox* g_targetDetails = NULL;

// The personality block under the skill controls on the Character page. The
// dropdown and Previous/Next only stage a choice; Apply writes it.
MyGUI::TextBox*  g_personalityHead = NULL;
MyGUI::TextBox*  g_personalityCurrent = NULL;
MyGUI::ComboBox* g_personalityCombo = NULL;
MyGUI::Button*   g_personalityApply = NULL;
MyGUI::delegates::IDelegate1<MyGUI::Widget*>* g_personalityApplyDelegate = NULL;

// One tick box and one label per switch, so a click lands on the box itself
// rather than on a row that then needs a second button pressed.
MyGUI::Button*   g_flagBoxes[16] = { NULL };
MyGUI::TextBox*  g_flagLabels[16] = { NULL };
MyGUI::delegates::IDelegate1<MyGUI::Widget*>* g_flagBoxDelegates[16] = { NULL };

MyGUI::delegates::IDelegate2<MyGUI::Window*, const std::string&>* g_closeDelegate = NULL;
MyGUI::delegates::IDelegate1<MyGUI::Widget*>* g_bonusAddDelegate = NULL;
MyGUI::delegates::IDelegate1<MyGUI::Widget*>* g_bonusDelDelegate = NULL;
MyGUI::delegates::IDelegate1<MyGUI::Widget*>* g_revertDelegate = NULL;
MyGUI::delegates::IDelegate1<MyGUI::Widget*>* g_statApplyDelegate = NULL;
MyGUI::delegates::IDelegate2<MyGUI::ComboBox*, size_t>* g_raceDelegate = NULL;
MyGUI::delegates::IDelegate2<MyGUI::ComboBox*, size_t>* g_statComboDelegate = NULL;
MyGUI::delegates::IDelegate2<MyGUI::ComboBox*, size_t>* g_copyComboDelegate = NULL;
MyGUI::delegates::IDelegate2<MyGUI::ScrollBar*, size_t>* g_bonusBarDelegate = NULL;
MyGUI::delegates::IDelegate2<MyGUI::ScrollBar*, size_t>* g_statBarDelegate = NULL;
MyGUI::delegates::IDelegate1<MyGUI::Widget*>* g_copyDelegate = NULL;
MyGUI::delegates::IDelegate1<MyGUI::Widget*>* g_filterDelegate = NULL;
MyGUI::delegates::IDelegate1<MyGUI::Widget*>* g_copyTraitsDelegate = NULL;
MyGUI::delegates::IDelegate1<MyGUI::Widget*>* g_jumpRaceDelegate = NULL;
MyGUI::delegates::IDelegate1<MyGUI::Widget*>* g_editAppearanceDelegate = NULL;
MyGUI::delegates::IDelegate1<MyGUI::Widget*>* g_bonusSearchDelegate = NULL;
MyGUI::delegates::IDelegate1<MyGUI::Widget*>* g_copySearchDelegate = NULL;
MyGUI::delegates::IDelegate1<MyGUI::Widget*>* g_statSearchDelegate = NULL;
MyGUI::delegates::IDelegate1<MyGUI::Widget*>* g_partApplyDelegate = NULL;
MyGUI::delegates::IDelegate2<MyGUI::ListBox*, size_t>* g_partListDelegate = NULL;
MyGUI::delegates::IDelegate1<MyGUI::Widget*>* g_realOnlyDelegate = NULL;
MyGUI::delegates::IDelegate1<MyGUI::Widget*>* g_hitApplyDelegate = NULL;
MyGUI::delegates::IDelegate1<MyGUI::Widget*>* g_flagToggleDelegate = NULL;
MyGUI::delegates::IDelegate2<MyGUI::ListBox*, size_t>* g_flagListDelegate = NULL;

bool g_open = false;

// The bonuses shown in the list, in the order they are drawn, so a click on
// row N resolves to the skill on row N without asking the engine again.

// Edits since the last save. Flushed when the window closes, because writing
// the file on every keystroke would hit the disk far more often than it needs.
bool g_dirty = false;

// Stored overrides are applied the first time the world is reachable, not at
// startPlugin: race data does not exist yet when the plugin loads, so applying
// there would silently do nothing.
bool g_overridesApplied = false;

// Whether a copy also carries the physical traits -- blood, healing, speed,
// hunger and the engine's flags. On by default: copying a race that still
// bleeds like the old one is rarely what was meant.
bool g_copyWithTraits = true;

// The character the window is currently describing. Compared every frame
// against the live selection so the panels follow the player's clicks.
Character* g_shownCharacter = NULL;

// Whether the race dropdowns hide the placeholder and test entries. On by
// default: of nearly two thousand definitions only a fraction are races anyone
// can meet, and the rest bury them.
bool g_realRacesOnly = true;

// The body parts shown, in draw order, so a click on row N resolves without
// asking the engine again.
std::vector<BodyPart> g_shownParts;

// Declared ahead of the handlers that use it; defined below with the rest of
// the list helpers.
int SelectedBonusIndex();
void RefreshBonusList();
void RefreshSubject();
void RefreshFlagList();
int StatAtRow(const std::vector<int>& visible, int row);
void RefreshRaceDetails();
void RefreshPartList();
void RefreshRealOnlyToggle();
void RefreshBlood();
void RefreshSkillList();
void RefreshStatCombo(MyGUI::ComboBox* combo, MyGUI::EditBox* search,
                      std::vector<int>* visible);
bool SelectRaceByName(const std::string& raceName);
void RefreshRaceBonusList();

std::string TextOf(const char* key, const char* fallback)
{
    return Text(key, fallback);
}

// Indices into Races() for the rows currently in the dropdowns.
//
// Kenshi ships close to two thousand race definitions -- every animal, every
// variant -- so an unfiltered dropdown is a wall nobody can find anything in.
// The combos show a filtered view, and this maps a combo row back to the race
// it stands for.
std::vector<int> g_visibleRaces;

// The same idea for the two skill dropdowns. Thirty-six entries is a shorter
// list than the races, but a player looking for "smithing" should not have to
// know whether it is filed under weapon or armour.
std::vector<int> g_visibleBonusStats;
std::vector<int> g_visibleStats;

// The copy source has its own filtered view of the races, independent of the
// race being edited: narrowing one should never move the other.
std::vector<int> g_visibleCopyRaces;

std::string ToLowerCopy(const std::string& text)
{
    std::string out(text);
    for (size_t i = 0; i < out.size(); ++i)
    {
        out[i] = (char)tolower((unsigned char)out[i]);
    }
    return out;
}

// Resolves a combo row to a race, through the filter.
const RaceEntry* RaceAtRow(int row)
{
    if (row < 0 || row >= (int)g_visibleRaces.size())
    {
        return NULL;
    }

    const std::vector<RaceEntry>& races = Races(false);
    const int index = g_visibleRaces[row];
    if (index < 0 || index >= (int)races.size())
    {
        return NULL;
    }
    return &races[index];
}

// The race selected in the copy-from picker on the left. Its own visible list
// is separate from the target's, so narrowing one never moves the other.
const RaceEntry* CopySourceEntry();

RaceData* CopySourceRace()
{
    const RaceEntry* entry = CopySourceEntry();
    return (entry != NULL) ? entry->race : NULL;
}

std::string CopySourceName()
{
    const RaceEntry* entry = CopySourceEntry();
    return (entry != NULL) ? entry->name : std::string();
}

RaceData* CurrentRace()
{
    const RaceEntry* entry = RaceAtRow(ComboSelection(g_raceCombo));
    return (entry != NULL) ? entry->race : NULL;
}

std::string CurrentRaceName()
{
    const RaceEntry* entry = RaceAtRow(ComboSelection(g_raceCombo));
    return (entry != NULL) ? entry->name : std::string();
}

// Delegates, so a skill the engine names but Stats() dropped still shows its
// real name instead of a bare "?".
std::string StatNameOf(int stat)
{
    return StatDisplayName(stat);
}

void SetStatus(const std::string& message, const MyGUI::Colour& colour = kTextPrimary)
{
    SetText(g_status, message);
    SetTextColour(g_status, colour);
}

// A multiplier and a scrollbar position are different things: the bar counts
// whole steps, so the value is scaled into a step index and back. This is the
// only reason bonus.steps exists in the config.
int BonusToStep(float value)
{
    const Config& config = GetConfig();
    const float span = config.bonusMax - config.bonusMin;
    if (span <= 0.0f)
    {
        return 0;
    }

    int step = (int)(((value - config.bonusMin) / span) * (float)config.bonusSteps + 0.5f);
    if (step < 0) { step = 0; }
    if (step > config.bonusSteps) { step = config.bonusSteps; }
    return step;
}

float StepToBonus(int step)
{
    const Config& config = GetConfig();
    if (config.bonusSteps < 1)
    {
        return config.bonusMin;
    }
    const float span = config.bonusMax - config.bonusMin;
    return config.bonusMin + (span * (float)step) / (float)config.bonusSteps;
}

// Stats are whole-ish numbers on a 0..200 range, so the bar maps one step per
// unit and needs no scaling beyond the offset.
int StatToStep(float value)
{
    const Config& config = GetConfig();
    int step = (int)(value - config.statMin + 0.5f);
    const int range = (int)(config.statMax - config.statMin);
    if (step < 0) { step = 0; }
    if (step > range) { step = range; }
    return step;
}

float StepToStat(int step)
{
    const Config& config = GetConfig();
    return config.statMin + (float)step;
}

// The list is rebuilt from the engine every time rather than patched, so what
// the player sees is always what the race actually carries.
// Everything that describes the race now selected, rebuilt from the engine
// rather than patched, so what the player sees is what the race actually
// carries. The bonuses themselves are the skill bonuses page's list.
void RefreshBonusList()
{
    SetEnabled(g_bonusDel, false);
    ClearListSelection(g_raceBonusList);

    // Every panel follows the race, rather than waiting for an edit. The part
    // list was only being rebuilt after a write, which is why it stayed empty.
    RefreshRaceDetails();
    RefreshPartList();
    RefreshFlagList();
    RefreshBlood();
    RefreshRaceBonusList();

    const std::string raceName = CurrentRaceName();
    SetEnabled(g_revert, !raceName.empty() && HasOriginal(raceName));
}

// The selected bonus, or -1. The placeholder row is not a bonus, so a selection
// that lands on it reports nothing selected.
//
// The list lives on the skill bonuses page now: page one compares two races and
// copies one onto the other, and every edit happens where the controls are.
int SelectedBonusIndex()
{
    const int index = ListSelection(g_raceBonusList);
    if (index < 0 || index >= (int)g_shownRaceBonuses.size())
    {
        return -1;
    }
    return index;
}

// The bonus page's own sheet: every bonus the selected race carries, beside
// the controls that edit one. Same shape as the character page -- read on the
// right, click a row, edit it on the left.
void RefreshRaceBonusList()
{
    g_shownRaceBonuses.clear();

    RaceData* race = CurrentRace();
    std::vector<std::string> rows;

    if (race != NULL)
    {
        g_shownRaceBonuses = ReadBonuses(race);
        for (size_t i = 0; i < g_shownRaceBonuses.size(); ++i)
        {
            char row[160] = { 0 };
            _snprintf_s(row, sizeof(row), _TRUNCATE, "%-28s x%.3f",
                        StatNameOf(g_shownRaceBonuses[i].stat).c_str(),
                        g_shownRaceBonuses[i].value);
            rows.push_back(row);
        }
    }

    if (rows.empty())
    {
        rows.push_back(TextOf("bonus.empty", "(this race has no bonuses)"));
    }

    FillList(g_raceBonusList, rows);

    const std::string raceName = CurrentRaceName();
    SetText(g_raceBonusHead, raceName.empty()
        ? TextOf("heading.raceBonuses", "THIS RACE'S BONUSES")
        : Fill(TextOf("heading.raceBonusesOf", "BONUSES OF {name}"),
               raceName, "", 0));
}

// Clicking a row loads that bonus into the controls on the left, exactly as
// clicking a skill does on the character page.
void OnRaceBonusSelected(MyGUI::ListBox* /*sender*/, size_t /*index*/)
{
    const int index = ListSelection(g_raceBonusList);
    SetEnabled(g_bonusDel, index >= 0 && index < (int)g_shownRaceBonuses.size());

    if (index < 0 || index >= (int)g_shownRaceBonuses.size())
    {
        return;
    }

    const BonusEntry& bonus = g_shownRaceBonuses[index];
    SetInputText(g_bonusValue, FormatFloat(bonus.value, 3));
    SetScrollPosition(g_bonusBar, BonusToStep(bonus.value));

    // Clearing the search guarantees the bonus's own skill is in the dropdown:
    // a filter left from an earlier search could otherwise hide the row just
    // clicked.
    SetInputText(g_bonusSearch, "");
    RefreshStatCombo(g_bonusStat, g_bonusSearch, &g_visibleBonusStats);

    for (size_t i = 0; i < g_visibleBonusStats.size(); ++i)
    {
        if (g_visibleBonusStats[i] == bonus.stat)
        {
            std::vector<std::string> names;
            for (size_t j = 0; j < g_visibleBonusStats.size(); ++j)
            {
                names.push_back(StatNameOf(g_visibleBonusStats[j]));
            }
            FillCombo(g_bonusStat, names, (int)i);
            break;
        }
    }
}

// Every skill the selected character has, with its value. The controls on the
// left edit one at a time; this is the sheet you read to decide which.
void RefreshSkillList()
{
    Character* character = SelectedCharacter();
    std::vector<std::string> rows;

    if (character != NULL)
    {
        const std::vector<StatEntry>& stats = Stats();
        for (size_t i = 0; i < stats.size(); ++i)
        {
            float value = 0.0f;
            if (!ReadStat(character, stats[i].stat, &value))
            {
                continue;
            }

            char row[160] = { 0 };
            _snprintf_s(row, sizeof(row), _TRUNCATE, "%-28s %.0f",
                        stats[i].name.c_str(), value);
            rows.push_back(row);
        }
    }

    if (rows.empty())
    {
        rows.push_back(TextOf("stats.noSelection",
                              "No character selected. Click one in the world."));
    }

    FillList(g_skillList, rows);
}

// Clicking a skill loads it into the editing controls, so the sheet doubles as
// the way to pick what to change.
void OnSkillListSelected(MyGUI::ListBox* /*sender*/, size_t /*index*/)
{
    const int row = ListSelection(g_skillList);
    const std::vector<StatEntry>& stats = Stats();
    if (row < 0 || row >= (int)stats.size())
    {
        return;
    }

    // Clearing the search guarantees the skill is in the dropdown: a filter
    // left from an earlier search could otherwise hide the row just clicked.
    SetInputText(g_statSearch, "");
    RefreshStatCombo(g_statCombo, g_statSearch, &g_visibleStats);

    for (size_t i = 0; i < g_visibleStats.size(); ++i)
    {
        if (g_visibleStats[i] == stats[row].stat)
        {
            std::vector<std::string> names;
            for (size_t j = 0; j < g_visibleStats.size(); ++j)
            {
                names.push_back(StatNameOf(g_visibleStats[j]));
            }
            FillCombo(g_statCombo, names, (int)i);
            break;
        }
    }

    RefreshSubject();
}

void RefreshSubject()
{
    Character* character = SelectedCharacter();
    g_shownCharacter = character;
    if (character == NULL)
    {
        SetText(g_subject, TextOf("stats.noSelection",
                                  "No character selected. Click one in the world."));
        SetText(g_subjectRace, "");
        SetEnabled(g_statApply, false);
        SetEnabled(g_jumpRace, false);
        SetEnabled(g_editAppearance, false);
        return;
    }

    const std::string name = CharacterName(character);
    SetText(g_subject, Fill(TextOf("stats.subject", "Editing: {name}"),
                            name, "", 0));
    SetTextColour(g_subject, kBrass);
    SetEnabled(g_statApply, true);
    SetEnabled(g_editAppearance, CanEditAppearance(character) == APPEARANCE_OK);

    // Which race this character belongs to -- and therefore which race the
    // left-hand panel would be editing on their behalf. Without it there is no
    // way to tell from this window whose bonuses are whose.
    const std::string raceName = RaceNameOf(RaceOf(character));
    if (raceName.empty())
    {
        SetText(g_subjectRace, TextOf("stats.unknownRace", "Race: unknown"));
        SetTextColour(g_subjectRace, kMuted);
        SetEnabled(g_jumpRace, false);
    }
    else
    {
        SetText(g_subjectRace, Fill(TextOf("stats.race", "Race: {name}"),
                                    raceName, "", 0));
        SetTextColour(g_subjectRace, kMuted);
        SetEnabled(g_jumpRace, true);
    }

    // Show the character's current value for whichever skill is picked, so the
    // number in the box is a starting point rather than a blank to guess at.
    const int stat = StatAtRow(g_visibleStats, ComboSelection(g_statCombo));
    if (stat > 0)
    {
        float value = 0.0f;
        if (ReadStat(character, stat, &value))
        {
            SetInputText(g_statValue, FormatFloat(value, 1));
            SetScrollPosition(g_statBar, StatToStep(value));
        }
    }
}

// Rebuilds both race dropdowns from the current filter, keeping the selected
// race selected when it survives the filter.
//
// The filter is a plain case-insensitive substring match on the race name,
// which is what someone typing "green" or "skeleton" expects.
void RefreshRaceCombos()
{
    const std::string keyword = ToLowerCopy(GetInputText(g_filter));

    // Remember what was selected so a filter that still contains it does not
    // move the selection out from under the player.
    const RaceEntry* previous = RaceAtRow(ComboSelection(g_raceCombo));
    const std::string previousName = (previous != NULL) ? previous->name : std::string();

    g_visibleRaces.clear();
    std::vector<std::string> names;

    const std::vector<RaceEntry>& races = Races(false);
    int selected = -1;
    for (size_t i = 0; i < races.size(); ++i)
    {
        if (g_realRacesOnly && !races[i].playable)
        {
            continue;
        }
        if (!keyword.empty() &&
            ToLowerCopy(races[i].name).find(keyword) == std::string::npos)
        {
            continue;
        }

        if (!previousName.empty() && races[i].name == previousName)
        {
            selected = (int)g_visibleRaces.size();
        }
        g_visibleRaces.push_back((int)i);
        names.push_back(races[i].name);
    }

    if (selected < 0 && !names.empty())
    {
        selected = 0;
    }

    FillCombo(g_raceCombo, names, selected);

    char count[16] = { 0 };
    _snprintf_s(count, sizeof(count), _TRUNCATE, "%d", (int)names.size());
    SetStatus(Fill(TextOf("status.filtered", "{count} races listed."),
                   "", "", (int)names.size()));
}

// MyGUI's ListBox and ComboBox carry no tick box this window can rely on, so
// the toggle is a button whose caption states its own state. Less pretty than a
// checkbox, and unambiguous.
void RefreshTraitToggle()
{
    SetText(g_copyTraits, g_copyWithTraits
        ? TextOf("copy.traitsOn",  "[X]  With blood and traits")
        : TextOf("copy.traitsOff", "[  ]  Bonuses only"));
}

void OnToggleTraits(MyGUI::Widget* /*sender*/)
{
    g_copyWithTraits = !g_copyWithTraits;
}

void ApplyLabels()
{
    SetBodyText(g_pageHelp1, TextOf("help.overview",
        "Pick a race on the left to see everything the engine knows about it on "
        "the right. Every edit in this window changes the race itself, so it "
        "reaches every character of that race in the world."));
    SetTextColour(g_pageHelp1, kMuted);

    SetBodyText(g_pageHelp2, TextOf("help.bonuses",
        "A bonus is a MULTIPLIER the engine applies to a skill: 1.15 means "
        "fifteen percent more, 1.0 means unchanged. Copying replaces the "
        "selected race's bonuses with another race's."));
    SetTextColour(g_pageHelp2, kMuted);

    SetBodyText(g_pageHelp3, TextOf("help.parts",
        "Health is how much damage that part takes before it is crippled. Hit "
        "weight is relative, not a percentage: the engine divides one part's "
        "weight by the total, so raising one lowers every other part's share."));
    SetTextColour(g_pageHelp3, kMuted);

    SetBodyText(g_pageHelp4, TextOf("help.flags",
        "Every box reads as a permission: ticked means the race CAN do it. The "
        "engine stores some of these the other way round -- its field is called "
        "'no hats' -- and the window flips them so a tick never has to be read "
        "backwards. Untick 'can wear hats' and that race stops equipping "
        "helmets."));
    SetTextColour(g_pageHelp4, kMuted);

    SetBodyText(g_pageHelp5, TextOf("help.character",
        "This page edits the character you have selected in the world, not "
        "their race. Skills live in your save, so these changes stay with that "
        "one person."));
    SetTextColour(g_pageHelp5, kMuted);

    SetText(g_realOnlyLabel, TextOf("filter.realOnlyLabel",
                                    "Hide test and placeholder races"));
    SetTextColour(g_realOnlyLabel, kMuted);

    SetText(g_copyTraitsLabel, TextOf("copy.traitsLabel",
                                      "Also copy blood, speed and traits"));
    SetTextColour(g_copyTraitsLabel, kMuted);

    SetText(g_jumpRace, TextOf("stats.jump", "Edit this race"));
    SetTextColour(g_jumpRace, kBrass);

    SetText(g_editAppearance, TextOf("appearance.button", "Edit Appearance"));
    SetTextColour(g_editAppearance, kBrass);

    SetText(g_detailsHead, TextOf("heading.details", "WHAT THIS RACE IS"));
    SetTextColour(g_detailsHead, kBrass);

    SetText(g_partHead, TextOf("heading.parts", "BODY PART HEALTH  (global)"));
    SetTextColour(g_partHead, kBrass);

    SetText(g_partLabel, TextOf("label.partValue", "Health  (1-500)"));
    SetTextColour(g_partLabel, kMuted);

    SetText(g_partApply, TextOf("part.apply", "Set part health"));
    SetTextColour(g_partApply, kAmber);

    SetText(g_hitLabel, TextOf("label.hitValue", "Hit weight  (relative)"));
    SetTextColour(g_hitLabel, kMuted);

    SetText(g_hitApply, TextOf("part.applyHit", "Set hit weight"));
    SetTextColour(g_hitApply, kAmber);

    SetText(g_bloodHead, TextOf("heading.blood", "BLOOD  (whole body, global)"));
    SetTextColour(g_bloodHead, kBrass);

    SetText(g_bloodMinLabel, TextOf("label.bloodMin", "Minimum  (newborn)"));
    SetTextColour(g_bloodMinLabel, kMuted);

    SetText(g_bloodMaxLabel, TextOf("label.bloodMax", "Maximum  (adult)"));
    SetTextColour(g_bloodMaxLabel, kMuted);

    SetText(g_bloodApply, TextOf("blood.apply", "Set blood range"));
    SetTextColour(g_bloodApply, kAmber);

    SetText(g_skillListHead, TextOf("heading.skillList", "EVERY SKILL, AND ITS VALUE"));
    SetTextColour(g_skillListHead, kBrass);

    SetText(g_personalityHead, TextOf("personality.heading", "PERSONALITY"));
    SetTextColour(g_personalityHead, kBrass);

    SetText(g_personalityApply, TextOf("personality.apply", "APPLY"));
    SetTextColour(g_personalityApply, kAmber);
    RefreshRealOnlyToggle();

    SetText(g_editHead, TextOf("heading.edit", "EDIT RACE"));
    SetTextColour(g_editHead, kBrass);

    SetText(g_raceLabel, TextOf("label.race", "Race"));
    SetTextColour(g_raceLabel, kMuted);

    SetText(g_bonusLabel, TextOf("label.bonusSkill", "Skill to give a bonus"));
    SetTextColour(g_bonusLabel, kMuted);

    SetText(g_bonusValueLabel, TextOf("label.bonusValue", "Multiplier  (1.0 = unchanged)"));
    SetTextColour(g_bonusValueLabel, kMuted);

    SetText(g_copyLabel, TextOf("label.copyFrom", "Copy from this race"));
    SetTextColour(g_copyLabel, kMuted);

    SetText(g_statLabel, TextOf("label.statSkill", "Skill to edit"));
    SetTextColour(g_statLabel, kMuted);

    SetText(g_statValueLabel, TextOf("label.statValue", "Value  (0-200)"));
    SetTextColour(g_statValueLabel, kMuted);

    // One caption for all three search buttons: they do the same thing to the
    // list directly beneath them.
    SetText(g_bonusSearchGo, TextOf("filter.button", "Search"));
    SetTextColour(g_bonusSearchGo, kBrass);

    SetText(g_copySearchGo, TextOf("filter.button", "Search"));
    SetTextColour(g_copySearchGo, kBrass);

    SetText(g_statSearchGo, TextOf("filter.button", "Search"));
    SetTextColour(g_statSearchGo, kBrass);
}

// Selects the race of the character on the right in the race dropdown on the
// left, clearing the search filter so the race is reachable even when the
// current filter would hide it.
// Points the race picker at one race by name, clearing the search first so the
// race is reachable even when the current filter would hide it.
//
// Every race panel reads from this combo, so moving it is what makes the bonus
// list, the body parts and the blood range describe that race.
bool SelectRaceByName(const std::string& raceName)
{
    if (raceName.empty())
    {
        return false;
    }

    SetInputText(g_filter, "");
    RefreshRaceCombos();

    for (size_t i = 0; i < g_visibleRaces.size(); ++i)
    {
        const RaceEntry* entry = RaceAtRow((int)i);
        if (entry != NULL && entry->name == raceName)
        {
            std::vector<std::string> names;
            for (size_t j = 0; j < g_visibleRaces.size(); ++j)
            {
                const RaceEntry* row = RaceAtRow((int)j);
                names.push_back((row != NULL) ? row->name : std::string());
            }
            FillCombo(g_raceCombo, names, (int)i);

            RaceData* race = CurrentRace();
            if (race != NULL)
            {
                RememberOriginal(raceName, race);
            }
            RefreshBonusList();
            return true;
        }
    }
    return false;
}

void OnJumpToRace(MyGUI::Widget* /*sender*/)
{
    const std::string raceName = RaceNameOf(RaceOf(SelectedCharacter()));
    if (!SelectRaceByName(raceName))
    {
        SetStatus(TextOf("status.unknownRace",
                         "That character's race is not in the list."));
        return;
    }

    SetStatus(Fill(TextOf("status.jumped", "Showing bonuses for {name}."),
                   raceName, "", 0));
}

// Formats one labelled value as a line of the details panel, padded so the
// values line up in a column rather than drifting with the label lengths.
std::string DetailLine(const std::string& label, const std::string& value)
{
    std::string out = "  " + label;
    while (out.size() < 26)
    {
        out += " ";
    }
    return out + value + "\n";
}

std::string DetailFloat(const std::string& label, float value)
{
    return DetailLine(label, FormatFloat(value, 2));
}

std::string DetailFlag(const std::string& label, bool value)
{
    return DetailLine(label, value ? TextOf("details.yes", "yes")
                                   : TextOf("details.no", "no"));
}

// Everything the engine knows about the selected race, in one readable block.
//
// This is what makes a copy an informed decision rather than a guess: the two
// races' numbers are the thing being copied, and until they were on screen the
// player had no way to see what they were about to overwrite.
// The report describes the SOURCE race: it is what a copy would bring across,
// and the panel sits under the source picker for that reason. The target's own
// contents are the bonus list on the other side.
// Splits the built report into one row per line.
//
// The report is a list of lines, so it belongs in a ListBox rather than in a
// static edit box: Kenshi's edit skins have no scrollbar, and a report that
// outgrows its panel was being clipped with no way to reach the rest. A list
// scrolls for free, and it is what the bonus and skill sheets already use.
void FillReport(MyGUI::ListBox* list, const std::string& text)
{
    std::vector<std::string> rows;
    std::string row;
    for (size_t i = 0; i < text.size(); ++i)
    {
        if (text[i] == '\n')
        {
            rows.push_back(row);
            row.clear();
            continue;
        }
        if (text[i] != '\r')
        {
            row += text[i];
        }
    }
    if (!row.empty())
    {
        rows.push_back(row);
    }
    FillList(list, rows);
}

// One report, either side. Page one puts the source's on the left and the
// target's on the right so a copy can be judged before it is made; the numbers
// are read-only here, and every one of them is editable on its own page.
void RefreshDetailsInto(MyGUI::ListBox* widget, RaceData* race)
{
    if (race == NULL)
    {
        FillReport(widget, TextOf("details.none", "No race selected."));
        return;
    }

    const RaceTraits t = ReadTraits(race);
    if (!t.valid)
    {
        FillReport(widget, TextOf("details.unreadable",
                                       "This race's data could not be read."));
        return;
    }

    std::string text;

    text += TextOf("details.bodyHead", "BODY") + "\n";
    text += DetailFloat(TextOf("details.bloodMin", "Blood, minimum"), t.originalBloodMin);
    text += DetailFloat(TextOf("details.bloodMax", "Blood, maximum"), t.originalBloodMax);
    text += DetailFloat(TextOf("details.healRate", "Heal rate"), t.healRate);
    text += DetailFloat(TextOf("details.bleedRate", "Bleed rate"), t.bleedRate);
    text += DetailFloat(TextOf("details.hungerRate", "Hunger rate"), t.hungerRate);
    text += DetailFloat(TextOf("details.vision", "Vision multiplier"), t.visionMultiplier);
    text += "\n";

    text += TextOf("details.moveHead", "MOVEMENT") + "\n";
    text += DetailFloat(TextOf("details.walkSpeed", "Walk speed"), t.walkSpeed);
    text += DetailFloat(TextOf("details.swimSpeed", "Swim speed"), t.swimSpeed);
    text += DetailFloat(TextOf("details.runMin", "Run speed, min skill"), t.runSpeedMinSkill);
    text += DetailFloat(TextOf("details.runMax", "Run speed, max skill"), t.runSpeedMaxSkill);
    text += DetailFloat(TextOf("details.waterAvoid", "Water avoidance"), t.waterAvoidance);
    text += "\n";

    text += TextOf("details.natureHead", "NATURE") + "\n";
    text += DetailFlag(TextOf("details.robot", "Robot"), t.robot);
    text += DetailFlag(TextOf("details.vampiric", "Vampiric"), t.vampiric);
    text += DetailFlag(TextOf("details.gigantic", "Gigantic"), t.gigantic);
    text += DetailFlag(TextOf("details.swims", "Swims"), t.swims);
    text += DetailFlag(TextOf("details.carriable", "Can be carried"), t.carriable);
    text += DetailFlag(TextOf("details.singleGender", "Single gender"), t.singleGender);
    text += DetailFlag(TextOf("details.indoors", "Can go indoors"), t.canGoIndoors);
    text += DetailFlag(TextOf("details.noHats", "Wears no hats"), t.noHats);
    text += DetailFlag(TextOf("details.noShirts", "Wears no shirts"), t.noShirts);
    text += DetailFlag(TextOf("details.noShoes", "Wears no shoes"), t.noShoes);
    text += "\n";

    // The per-part health: a Shek's 125 a limb against a Greenlander's 100.
    // These live in the race's own "combat anatomy" list rather than in
    // RaceData, which is why they are read through GameData -- and why a copy
    // that moved only RaceData fields would leave them behind.
    const std::vector<BodyPart> parts = ReadAnatomy(race);
    if (!parts.empty())
    {
        text += TextOf("details.anatomyHead",
                       "BODY PARTS  (hit chance / health)") + "\n";
        // The weight alone says nothing: the engine divides it by the total
        // across the race, so the percentage is what the player can reason
        // about. Both are shown -- the percentage to understand, the weight
        // because that is what gets edited.
        const int total = TotalHitWeight(parts);
        for (size_t i = 0; i < parts.size(); ++i)
        {
            const float percent = (total > 0)
                ? (100.0f * (float)parts[i].hitChance / (float)total)
                : 0.0f;

            char numbers[96] = { 0 };
            _snprintf_s(numbers, sizeof(numbers), _TRUNCATE,
                        "%d hp    %.1f%% hit  (%d)",
                        parts[i].health, percent, parts[i].hitChance);
            text += DetailLine(parts[i].name, numbers);
        }
        text += "\n";
    }

    text += TextOf("details.combatHead", "COMBAT") + "\n";
    text += DetailLine(TextOf("details.attackSlots", "Extra attack slots"),
                       FormatFloat((float)t.extraAttackSlots, 0));
    text += DetailLine(TextOf("details.firstAid", "First-aid skill"),
                       StatNameOf(t.firstAidSkill));
    text += "\n";

    // The bonuses are listed beside the traits rather than only in the list on
    // the left, so one glance covers everything a copy would carry across.
    const std::vector<BonusEntry> bonuses = ReadBonuses(race);
    char count[16] = { 0 };
    _snprintf_s(count, sizeof(count), _TRUNCATE, "%d", (int)bonuses.size());
    text += Fill(TextOf("details.bonusHead", "SKILL BONUSES ({count})"),
                 "", "", (int)bonuses.size()) + "\n";

    if (bonuses.empty())
    {
        text += TextOf("details.noBonuses", "  none") + "\n";
    }
    else
    {
        for (size_t i = 0; i < bonuses.size(); ++i)
        {
            // Only the ones that actually change something: a race carrying
            // thirty-eight multipliers of exactly 1.0 has no bonuses worth
            // reading, and listing them all buries the ones that matter.
            if (bonuses[i].value == 1.0f)
            {
                continue;
            }
            text += DetailLine(StatNameOf(bonuses[i].stat),
                               "x" + FormatFloat(bonuses[i].value, 3));
        }
    }

    FillReport(widget, text);
}

// The left panel follows the copy source, the right one the race being edited.
void RefreshRaceDetails()
{
    RefreshDetailsInto(g_details, CopySourceRace());
    RefreshDetailsInto(g_targetDetails, CurrentRace());
}

// Fills one skill dropdown from a search box, and records which skill each row
// stands for so a selection resolves without re-searching.
void RefreshStatCombo(MyGUI::ComboBox* combo, MyGUI::EditBox* search,
                      std::vector<int>* visible)
{
    const std::string keyword = ToLowerCopy(GetInputText(search));

    visible->clear();
    std::vector<std::string> names;

    const std::vector<StatEntry>& stats = Stats();
    for (size_t i = 0; i < stats.size(); ++i)
    {
        if (!keyword.empty() &&
            ToLowerCopy(stats[i].name).find(keyword) == std::string::npos)
        {
            continue;
        }
        visible->push_back(stats[i].stat);
        names.push_back(stats[i].name);
    }

    FillCombo(combo, names, names.empty() ? -1 : 0);
}

// Resolves a skill dropdown row to the skill it stands for.
int StatAtRow(const std::vector<int>& visible, int row)
{
    if (row < 0 || row >= (int)visible.size())
    {
        return 0;
    }
    return visible[row];
}

const RaceEntry* CopySourceEntry()
{
    const int row = ComboSelection(g_copyCombo);
    if (row < 0 || row >= (int)g_visibleCopyRaces.size())
    {
        return NULL;
    }

    const std::vector<RaceEntry>& races = Races(false);
    const int index = g_visibleCopyRaces[row];
    if (index < 0 || index >= (int)races.size())
    {
        return NULL;
    }
    return &races[index];
}

void RefreshCopyCombo()
{
    const std::string keyword = ToLowerCopy(GetInputText(g_copySearch));

    g_visibleCopyRaces.clear();
    std::vector<std::string> names;

    const std::vector<RaceEntry>& races = Races(false);
    for (size_t i = 0; i < races.size(); ++i)
    {
        if (g_realRacesOnly && !races[i].playable)
        {
            continue;
        }
        if (!keyword.empty() &&
            ToLowerCopy(races[i].name).find(keyword) == std::string::npos)
        {
            continue;
        }
        g_visibleCopyRaces.push_back((int)i);
        names.push_back(races[i].name);
    }

    FillCombo(g_copyCombo, names, names.empty() ? -1 : 0);
}

void OnCopySearch(MyGUI::Widget* /*sender*/)
{
    LogDebug("Source search clicked, text: '" + GetInputText(g_copySearch) + "'.");
    RefreshCopyCombo();
    RefreshRaceDetails();
}

// Picking a different source changes what the report describes.
void OnCopySourceChanged(MyGUI::ComboBox* /*sender*/, size_t /*index*/)
{
    RefreshRaceDetails();
}

void OnBonusSearch(MyGUI::Widget* /*sender*/)
{
    RefreshStatCombo(g_bonusStat, g_bonusSearch, &g_visibleBonusStats);
}

void OnStatSearch(MyGUI::Widget* /*sender*/)
{
    RefreshStatCombo(g_statCombo, g_statSearch, &g_visibleStats);
    RefreshSubject();
}

void OnFilterChanged(MyGUI::Widget* /*sender*/)
{
    LogDebug("Target search clicked, text: '" + GetInputText(g_filter) + "'.");
    RefreshRaceCombos();
    RefreshBonusList();
}

void OnRaceChanged(MyGUI::ComboBox* /*sender*/, size_t /*index*/)
{
    RaceData* race = CurrentRace();
    const std::string name = CurrentRaceName();

    // The baseline is captured the first time a race is looked at, not when it
    // is first edited: by the time an edit lands, the shipped values are
    // already gone.
    if (race != NULL && !name.empty())
    {
        RememberOriginal(name, race);
    }

    RefreshBonusList();
    SetStatus(Fill(TextOf("status.raceSelected", "Showing bonuses for {name}."),
                   name, "", 0));
}

void OnBonusBarMoved(MyGUI::ScrollBar* sender, size_t position)
{
    const float value = StepToBonus((int)position);
    SetInputText(g_bonusValue, FormatFloat(value, 3));
}

void OnStatBarMoved(MyGUI::ScrollBar* sender, size_t position)
{
    const float value = StepToStat((int)position);
    SetInputText(g_statValue, FormatFloat(value, 1));
}

void OnStatComboChanged(MyGUI::ComboBox* /*sender*/, size_t /*index*/)
{
    RefreshSubject();
}

void OnAddBonus(MyGUI::Widget* /*sender*/)
{
    RaceData* race = CurrentRace();
    const std::string raceName = CurrentRaceName();
    if (race == NULL)
    {
        SetStatus(TextOf("status.noRace", "Pick a race first."));
        return;
    }

    const int stat = StatAtRow(g_visibleBonusStats, ComboSelection(g_bonusStat));
    if (stat <= 0)
    {
        SetStatus(TextOf("status.noStat", "Pick a skill first."));
        return;
    }

    float value = 1.0f;
    if (!ParseFloat(GetInputText(g_bonusValue), &value))
    {
        SetStatus(TextOf("status.badNumber", "That is not a number."));
        return;
    }

    RememberOriginal(raceName, race);

    if (!WriteBonus(race, stat, value))
    {
        SetStatus(TextOf("status.writeFailed", "The engine refused that edit."));
        return;
    }

    RecordOverride(raceName, race);
    g_dirty = true;

    RefreshBonusList();
    SetStatus(Fill(TextOf("status.bonusSet", "{name} now multiplies by {value}."),
                   StatNameOf(stat), FormatFloat(value, 3), 0));
}

void OnRemoveBonus(MyGUI::Widget* /*sender*/)
{
    RaceData* race = CurrentRace();
    const std::string raceName = CurrentRaceName();
    const int index = SelectedBonusIndex();

    if (race == NULL || index < 0)
    {
        SetStatus(TextOf("status.noBonus", "Pick a bonus from the list first."));
        return;
    }

    const int stat = g_shownRaceBonuses[index].stat;
    const std::string name = StatNameOf(stat);

    RememberOriginal(raceName, race);

    if (!RemoveBonus(race, stat))
    {
        SetStatus(TextOf("status.writeFailed", "The engine refused that edit."));
        return;
    }

    RecordOverride(raceName, race);
    g_dirty = true;

    RefreshBonusList();
    SetStatus(Fill(TextOf("status.bonusRemoved", "Removed the {name} bonus."),
                   name, "", 0));
}

// Copies the bonuses of the race picked in the copy dropdown onto the race
// selected at the top of the left panel.
//
// Replacing, not merging: after this the two races carry the same bonuses,
// which is what "copy" means. The target's baseline is captured first, so the
// revert button still leads back to what the target itself shipped with -- not
// to whatever it looked like before the copy.
// Blood is reported as not copying, and every step of the path reads and writes
// it correctly, so this says what the numbers actually were on each side.
void LogCopyAttempt(bool withTraits, const RaceTraits& from, const RaceTraits& to)
{
    char detail[192] = { 0 };
    _snprintf_s(detail, sizeof(detail), _TRUNCATE,
                "withTraits %d, source valid %d blood %.0f-%.0f, "
                "target valid %d blood %.0f-%.0f",
                withTraits ? 1 : 0,
                from.valid ? 1 : 0, from.originalBloodMin, from.originalBloodMax,
                to.valid ? 1 : 0, to.originalBloodMin, to.originalBloodMax);
    LogDebug("Copy: " + std::string(detail) + ".");
}

void LogCopyResult(const RaceTraits& to)
{
    char detail[96] = { 0 };
    _snprintf_s(detail, sizeof(detail), _TRUNCATE,
                "valid %d blood %.0f-%.0f",
                to.valid ? 1 : 0, to.originalBloodMin, to.originalBloodMax);
    LogDebug("Copy done, target now: " + std::string(detail) + ".");
}

void OnCopyBonuses(MyGUI::Widget* /*sender*/)
{
    RaceData* target = CurrentRace();
    const std::string targetName = CurrentRaceName();
    if (target == NULL || targetName.empty())
    {
        SetStatus(TextOf("status.noRace", "Pick a race first."));
        return;
    }

    const int row = ComboSelection(g_copyCombo);
    const std::vector<RaceEntry>& races = Races(false);
    if (row < 0 || row >= (int)g_visibleCopyRaces.size())
    {
        SetStatus(TextOf("status.noSource", "Pick a race to copy from."));
        return;
    }

    const int sourceIndex = g_visibleCopyRaces[row];
    if (sourceIndex < 0 || sourceIndex >= (int)races.size())
    {
        SetStatus(TextOf("status.noSource", "Pick a race to copy from."));
        return;
    }
    const RaceEntry& source = races[sourceIndex];
    if (source.race == target)
    {
        SetStatus(TextOf("status.sameRace",
                         "That is the same race. Pick a different source."));
        return;
    }

    RememberOriginal(targetName, target);

    LogCopyAttempt(g_copyWithTraits, ReadTraits(source.race),
                   ReadTraits(target));

    if (!CopyBonuses(source.race, target, g_copyWithTraits))
    {
        SetStatus(TextOf("status.writeFailed", "The engine refused that edit."));
        return;
    }

    LogCopyResult(ReadTraits(target));

    RecordOverride(targetName, target);
    g_dirty = true;

    // Part health is read once, when a character's medical system is built, so
    // writing it onto the race leaves everyone already standing there on their
    // old numbers. "Set part health" pushes the race's values onto the selected
    // character for exactly this reason, and a copy has to do the same or the
    // panel shows the new health while the body still has the old.
    std::string extra;
    Character* character = SelectedCharacter();
    if (character != NULL && RaceOf(character) == target)
    {
        const int applied = ApplyAnatomyToCharacter(character, target);
        extra = (applied > 0)
            ? TextOf("status.partApplied", " Applied to the selected character.")
            : TextOf("status.partFailed", " The selected character did not change.");
    }
    else
    {
        extra = TextOf("status.partLater",
                       " Characters already alive keep their old health; "
                       "new ones get this.");
    }

    RefreshBonusList();
    SetStatus(Fill(TextOf("status.copied", "Copied {name} bonuses onto {value}."),
                   source.name, targetName, 0) + extra);
}

// Rebuilds the body-part list for the selected race.
void RefreshPartList()
{
    g_shownParts.clear();

    RaceData* race = CurrentRace();
    std::vector<std::string> rows;

    if (race != NULL)
    {
        g_shownParts = ReadAnatomy(race);
        const int total = TotalHitWeight(g_shownParts);
        for (size_t i = 0; i < g_shownParts.size(); ++i)
        {
            const float percent = (total > 0)
                ? (100.0f * (float)g_shownParts[i].hitChance / (float)total)
                : 0.0f;

            char row[192] = { 0 };
            _snprintf_s(row, sizeof(row), _TRUNCATE, "%s   %d hp   %.1f%%",
                        g_shownParts[i].name.c_str(), g_shownParts[i].health,
                        percent);
            rows.push_back(row);
        }
    }

    if (rows.empty())
    {
        rows.push_back(TextOf("part.empty", "(this race has no body parts)"));
    }

    FillList(g_partList, rows);
    ClearListSelection(g_partList);
    SetEnabled(g_partApply, false);
    SetEnabled(g_hitApply, false);
}

// Picking a part loads its current health into the box, so editing starts from
// the real value rather than from a blank.
void OnPartSelected(MyGUI::ListBox* /*sender*/, size_t /*index*/)
{
    const int row = ListSelection(g_partList);
    if (row < 0 || row >= (int)g_shownParts.size())
    {
        SetEnabled(g_partApply, false);
        SetEnabled(g_hitApply, false);
        return;
    }

    SetEnabled(g_partApply, true);
    SetEnabled(g_hitApply, true);
    SetInputText(g_partValue, FormatFloat((float)g_shownParts[row].health, 0));
    SetInputText(g_hitValue, FormatFloat((float)g_shownParts[row].hitChance, 0));
}

// The toggle states what it currently does rather than what it would do, so
// there is no guessing which way it is set.
void RefreshRealOnlyToggle()
{
    SetText(g_realOnly, g_realRacesOnly
        ? TextOf("filter.realOnly", "[X]  Real races only")
        : TextOf("filter.allRaces", "[  ]  Every definition"));
}

void OnToggleRealOnly(MyGUI::Widget* /*sender*/)
{
    g_realRacesOnly = !g_realRacesOnly;
    RefreshRealOnlyToggle();
    RefreshRaceCombos();
    RefreshCopyCombo();
    RefreshBonusList();
}

// How each flag is shown.
//
// Three of them are stored as prohibitions -- noHats is true when the race
// CANNOT wear hats -- so the window inverts those and labels them as
// permissions. A tick box the player has to read backwards is one they will
// set the wrong way round.
struct FlagRow
{
    int flag;
    const char* key;
    const char* label;
    bool inverted;
};

const FlagRow kFlagRows[] =
{
    { FLAG_NO_HATS,        "flag.hats",    "Can wear hats / helmets", true  },
    { FLAG_NO_SHIRTS,      "flag.shirts",  "Can wear shirts / armour", true  },
    { FLAG_NO_SHOES,       "flag.shoes",   "Can wear boots",           true  },
    { FLAG_CAN_GO_INDOORS, "flag.indoors", "Can go indoors",           false },
    { FLAG_SWIMS,          "flag.swims",   "Can swim",                 false },
    { FLAG_CARRIABLE,      "flag.carry",   "Can be carried",           false },
    { FLAG_ROBOT,          "flag.robot",   "Is a robot",               false },
    { FLAG_VAMPIRIC,       "flag.vampire", "Is vampiric",              false },
    { FLAG_GIGANTIC,       "flag.giant",   "Is gigantic",              false },
    { FLAG_SINGLE_GENDER,  "flag.gender",  "Single gender",            false }
};
const int kFlagRowCount = 10;

// Ticks every box to match the race, showing permissions rather than the
// engine's prohibitions.
// Shows one page and hides the rest. The current page's button is disabled,
// which is the one state a plain button offers and is enough to say where you
// are.
void ShowPage(int page)
{
    if (page < 0 || page >= kPageCount)
    {
        return;
    }

    // Whether every page and button actually exists: a null here would make the
    // strip look dead while the handler ran perfectly.
    int pages = 0;
    int buttons = 0;
    for (int i = 0; i < kPageCount; ++i)
    {
        if (g_pages[i] != NULL) { ++pages; }
        if (g_tabButtons[i] != NULL) { ++buttons; }
    }

    char detail[64] = { 0 };
    _snprintf_s(detail, sizeof(detail), _TRUNCATE, "%d (pages %d, buttons %d)",
                page, pages, buttons);
    LogDebug("ShowPage " + std::string(detail) + ".");

    g_currentPage = page;
    for (int i = 0; i < kPageCount; ++i)
    {
        SetVisible(g_pages[i], i == page);
        if (i == page)
        {
            SetOutlineButtonColour(&g_tabs[i], kAmber, kAmber, kSumi);
        }
        else
        {
            SetOutlineButtonColour(&g_tabs[i], kLine, kMuted, kBgDark);
        }
    }
}

void OnTabClicked(MyGUI::Widget* sender)
{
    for (int i = 0; i < kPageCount; ++i)
    {
        if (g_tabButtons[i] == sender)
        {
            if (i == g_currentPage) { return; }
            char index[8] = { 0 };
            _snprintf_s(index, sizeof(index), _TRUNCATE, "%d", i);
            LogDebug("Tab clicked: " + std::string(index) + ".");
            ShowPage(i);
            return;
        }
    }

    // Reaching here means a click arrived from a widget that is not one of the
    // tab buttons -- worth knowing, because it says the strip is wired to
    // something else rather than that the click was lost.
    LogDebug("Tab click from an unknown widget.");
}

void RefreshFlagList()
{
    RaceData* race = CurrentRace();

    for (int i = 0; i < kFlagRowCount; ++i)
    {
        bool raw = false;
        const bool ok = (race != NULL) && ReadFlag(race, kFlagRows[i].flag, &raw);

        // noHats is true when the race CANNOT wear hats, so the inverted ones
        // are flipped before they are shown: a tick means "yes, it can".
        const bool shown = ok && (kFlagRows[i].inverted ? !raw : raw);

        SetTicked(g_flagBoxes[i], shown);
        SetEnabled(g_flagBoxes[i], ok);
        SetText(g_flagLabels[i], TextOf(kFlagRows[i].key, kFlagRows[i].label));
    }
}

// One handler for every box; which one was clicked comes from the widget.
void OnFlagBoxClicked(MyGUI::Widget* sender)
{
    RaceData* race = CurrentRace();
    const std::string raceName = CurrentRaceName();

    int index = -1;
    for (int i = 0; i < kFlagRowCount; ++i)
    {
        if (g_flagBoxes[i] == sender)
        {
            index = i;
            break;
        }
    }

    if (race == NULL || index < 0)
    {
        RefreshFlagList();
        return;
    }

    // Kenshi_TickBoxSkin does NOT toggle itself: it is a Button with a selected
    // state its owner maintains, which KenshiLua's settings window shows by
    // flipping it by hand in its own click handler. Reading the box here
    // returned the value from before the click, so the mod wrote back exactly
    // what was already there and nothing changed.
    //
    // The state is therefore read as the current one and inverted to get what
    // the player is asking for.
    const bool wanted = !IsTicked(g_flagBoxes[index]);
    const bool raw = kFlagRows[index].inverted ? !wanted : wanted;

    // The box shows the new state straight away; RefreshFlagList below would
    // set it from the race anyway, but only once the write succeeded.
    SetTicked(g_flagBoxes[index], wanted);

    RememberOriginal(raceName, race);

    if (!WriteFlag(race, kFlagRows[index].flag, raw))
    {
        SetStatus(TextOf("status.writeFailed", "The engine refused that edit."));
        RefreshFlagList();
        return;
    }

    RecordOverride(raceName, race);
    g_dirty = true;

    // The race field alone reaches nobody who already exists: the engine reads
    // it when it builds a character and hands it to the inventory section as
    // its enabled flag. So the slot is flipped on the selected character here,
    // and the change is visible without spawning anyone new.
    std::string extra;
    if (FlagHasSlot(kFlagRows[index].flag))
    {
        Character* character = SelectedCharacter();
        if (character != NULL && RaceOf(character) == race)
        {
            if (ApplySlotToCharacter(character, kFlagRows[index].flag, raw))
            {
                extra = TextOf("status.slotApplied",
                               " Slot updated on the selected character.");
            }
            else
            {
                extra = TextOf("status.slotFailed",
                               " The slot on the selected character did not change.");
            }
        }
        else
        {
            // Existing characters of that race keep the inventory they were
            // built with; saying so beats leaving the player to wonder.
            extra = TextOf("status.slotLater",
                           " Characters already in the world keep their slots; "
                           "new ones get this.");
        }
    }

    const std::string label = TextOf(kFlagRows[index].key, kFlagRows[index].label);
    RefreshFlagList();
    RefreshRaceDetails();
    SetStatus(Fill(wanted ? TextOf("status.flagOn",  "{name}: yes.")
                          : TextOf("status.flagOff", "{name}: no."),
                   label, "", 0) + extra);
}

// The two standalone tick boxes work the same way.
void OnRealOnlyClicked(MyGUI::Widget* /*sender*/)
{
    // Same as the flag boxes: the skin does not toggle itself.
    g_realRacesOnly = !IsTicked(g_realOnly);
    SetTicked(g_realOnly, g_realRacesOnly);
    RefreshRaceCombos();
    RefreshCopyCombo();
    RefreshBonusList();
}

void OnCopyTraitsClicked(MyGUI::Widget* /*sender*/)
{
    g_copyWithTraits = !IsTicked(g_copyTraits);
    SetTicked(g_copyTraits, g_copyWithTraits);
}

void OnApplyHitChance(MyGUI::Widget* /*sender*/)
{
    RaceData* race = CurrentRace();
    const std::string raceName = CurrentRaceName();
    const int row = ListSelection(g_partList);

    if (race == NULL || row < 0 || row >= (int)g_shownParts.size())
    {
        SetStatus(TextOf("status.noPart", "Pick a body part from the list first."));
        return;
    }

    float value = 0.0f;
    if (!ParseFloat(GetInputText(g_hitValue), &value))
    {
        SetStatus(TextOf("status.badNumber", "That is not a number."));
        return;
    }

    int weight = (int)(value + 0.5f);
    if (weight < 0) { weight = 0; }

    RememberOriginal(raceName, race);

    if (!WritePartHitChance(race, g_shownParts[row].index, weight))
    {
        SetStatus(TextOf("status.writeFailed", "The engine refused that edit."));
        return;
    }

    const std::string partName = g_shownParts[row].name;

    Character* hitCharacter = SelectedCharacter();
    if (hitCharacter != NULL && RaceOf(hitCharacter) == race)
    {
        ApplyAnatomyToCharacter(hitCharacter, race);
    }

    RefreshPartList();
    RefreshRaceDetails();
    SetStatus(Fill(TextOf("status.hitSet", "{name} hit weight set to {value}."),
                   partName, FormatFloat((float)weight, 0), 0));
}

// Loads the selected race's blood range into the two boxes.
void RefreshBlood()
{
    RaceData* race = CurrentRace();
    if (race == NULL)
    {
        SetInputText(g_bloodMin, "");
        SetInputText(g_bloodMax, "");
        SetEnabled(g_bloodApply, false);
        return;
    }

    const RaceTraits t = ReadTraits(race);
    if (!t.valid)
    {
        SetEnabled(g_bloodApply, false);
        return;
    }

    SetInputText(g_bloodMin, FormatFloat(t.originalBloodMin, 0));
    SetInputText(g_bloodMax, FormatFloat(t.originalBloodMax, 0));
    SetEnabled(g_bloodApply, true);
}

void OnApplyBlood(MyGUI::Widget* /*sender*/)
{
    RaceData* race = CurrentRace();
    const std::string raceName = CurrentRaceName();
    if (race == NULL)
    {
        SetStatus(TextOf("status.noRace", "Pick a race first."));
        return;
    }

    float minBlood = 0.0f;
    float maxBlood = 0.0f;
    if (!ParseFloat(GetInputText(g_bloodMin), &minBlood) ||
        !ParseFloat(GetInputText(g_bloodMax), &maxBlood))
    {
        SetStatus(TextOf("status.badNumber", "That is not a number."));
        return;
    }

    if (maxBlood < minBlood)
    {
        SetStatus(TextOf("status.bloodOrder",
                         "The maximum cannot be below the minimum."));
        return;
    }

    RememberOriginal(raceName, race);

    if (!WriteBloodRange(race, minBlood, maxBlood))
    {
        SetStatus(TextOf("status.writeFailed", "The engine refused that edit."));
        return;
    }

    RecordOverride(raceName, race);
    g_dirty = true;

    // Blood is set when a character is built, by interpolating this range over
    // their age, so everyone already alive keeps the old value. The selected
    // character gets the new one here rather than after a reload.
    std::string extra;
    Character* character = SelectedCharacter();
    if (character != NULL && RaceOf(character) == race)
    {
        extra = ReapplyBlood(character)
            ? TextOf("status.bloodApplied", " Applied to the selected character.")
            : TextOf("status.bloodFailed", " The selected character did not change.");
    }
    else
    {
        extra = TextOf("status.bloodLater",
                       " Characters already alive keep their blood; new ones get this.");
    }

    RefreshBlood();
    RefreshRaceDetails();
    SetStatus(TextOf("status.bloodSet", "Blood range updated.") + extra);
}

void OnApplyPart(MyGUI::Widget* /*sender*/)
{
    RaceData* race = CurrentRace();
    const std::string raceName = CurrentRaceName();
    const int row = ListSelection(g_partList);

    if (race == NULL || row < 0 || row >= (int)g_shownParts.size())
    {
        SetStatus(TextOf("status.noPart", "Pick a body part from the list first."));
        return;
    }

    float value = 0.0f;
    if (!ParseFloat(GetInputText(g_partValue), &value))
    {
        SetStatus(TextOf("status.badNumber", "That is not a number."));
        return;
    }

    const Config& config = GetConfig();
    int health = (int)(value + 0.5f);
    if (health < config.partHealthMin) { health = config.partHealthMin; }
    if (health > config.partHealthMax) { health = config.partHealthMax; }

    RememberOriginal(raceName, race);

    if (!WritePartHealth(race, g_shownParts[row].index, health))
    {
        SetStatus(TextOf("status.writeFailed", "The engine refused that edit."));
        return;
    }

    // Part health lives in the race's GameData rather than in RaceData, so it
    // is not covered by the bonus override store yet: the edit holds for this
    // session and is reported as such.
    // Part health is read when the medical system is built, so the character
    // already standing there keeps the old number until this pushes it over.
    std::string extra;
    Character* character = SelectedCharacter();
    if (character != NULL && RaceOf(character) == race)
    {
        const int applied = ApplyAnatomyToCharacter(character, race);
        extra = (applied > 0)
            ? TextOf("status.partApplied", " Applied to the selected character.")
            : TextOf("status.partFailed", " The selected character did not change.");
    }
    else
    {
        extra = TextOf("status.partLater",
                       " Characters already alive keep their old health; "
                       "new ones get this.");
    }

    // The list, the report and the boxes all describe what was just written,
    // so they are refreshed together rather than leaving a stale number on
    // screen beside a status line claiming it changed.
    RefreshPartList();
    RefreshRaceDetails();
    RefreshBlood();
    SetStatus(Fill(TextOf("status.partSet", "{name} set to {value} hp."),
                   g_shownParts[row].name, FormatFloat((float)health, 0), 0) + extra);
}

void OnRevert(MyGUI::Widget* /*sender*/)
{
    RaceData* race = CurrentRace();
    const std::string raceName = CurrentRaceName();

    if (race == NULL || !HasOriginal(raceName))
    {
        SetStatus(TextOf("status.noOriginal",
                         "No original values were recorded for this race."), kDanger);
        return;
    }

    if (!ReplaceBonuses(race, OriginalBonuses(raceName)))
    {
        SetStatus(TextOf("status.writeFailed", "The engine refused that edit."), kDanger);
        return;
    }

    // Traits go back too, when a baseline for them was recorded. A store from
    // before traits existed has none, and writing an empty set would leave the
    // race with no blood and no walk speed.
    if (OriginalTraits(raceName).valid)
    {
        WriteTraits(race, OriginalTraits(raceName));
    }

    // Body parts too. They live in GameData rather than RaceData, so they are
    // stored and restored separately from the traits above -- and a copy brings
    // them across, so leaving them would make "back to its original values"
    // untrue for the numbers most likely to have changed.
    const int parts = RestoreOriginalParts(raceName, race);

    // Reverting drops the override rather than storing the vanilla values as an
    // edit: the race goes back to being untouched, which is what the button
    // says it does.
    ForgetOverride(raceName);
    g_dirty = true;

    // Part health is read when a character's medical system is built, so the
    // race going back is not enough for anyone already standing there.
    std::string extra;
    if (parts > 0)
    {
        Character* character = SelectedCharacter();
        if (character != NULL && RaceOf(character) == race)
        {
            ApplyAnatomyToCharacter(character, race);
            extra = TextOf("status.partApplied",
                           " Applied to the selected character.");
        }
    }

    RefreshBonusList();
    SetStatus(Fill(TextOf("status.reverted", "{name} is back to its original values."),
                   raceName, "", 0) + extra, kDanger);
}

void OnApplyStat(MyGUI::Widget* /*sender*/)
{
    Character* character = SelectedCharacter();
    if (character == NULL)
    {
        SetStatus(TextOf("stats.noSelection",
                         "No character selected. Click one in the world."));
        return;
    }

    const int stat = StatAtRow(g_visibleStats, ComboSelection(g_statCombo));
    if (stat <= 0)
    {
        SetStatus(TextOf("status.noStat", "Pick a skill first."));
        return;
    }

    float value = 0.0f;
    if (!ParseFloat(GetInputText(g_statValue), &value))
    {
        SetStatus(TextOf("status.badNumber", "That is not a number."));
        return;
    }

    if (!WriteStat(character, stat, value))
    {
        SetStatus(TextOf("status.writeFailed", "The engine refused that edit."));
        return;
    }

    // Character stats are not saved by this mod: they live in the save game,
    // which is where the player expects a character's own skills to be.
    RefreshSubject();
    RefreshSkillList();
    SetStatus(Fill(TextOf("status.statSet", "{name} set to {value}."),
                   StatNameOf(stat), FormatFloat(value, 1), 0));
}

// Why a personality could not be read or written, in the status line's words.
// The fallbacks are CharTweak 2's own messages.
std::string PersonalityFailureText(PersonalityResult result)
{
    switch (result)
    {
    case PERSONALITY_NO_CHARACTER:
        return TextOf("personality.noSelection", "Select a character first.");
    case PERSONALITY_NO_STATE:
        return TextOf("personality.noState",
                      "Selected character has no personality state.");
    default:
        return TextOf("status.writeFailed", "The engine refused that edit.");
    }
}

// Shows the selected character's personality and stages it in the dropdown.
// Called when the shown character changes, so a choice staged for one person
// is never left waiting to be applied to the next.
void RefreshPersonality()
{
    int value = 0;
    const bool readable =
        ReadPersonality(SelectedCharacter(), &value) == PERSONALITY_OK;

    SetText(g_personalityCurrent,
            Fill(TextOf("personality.current", "Current: {value}"), "",
                 readable ? PersonalityLabel(value) : std::string("-"), 0));
    SetTextColour(g_personalityCurrent, kAmber);
    FillCombo(g_personalityCombo, PersonalityLabels(),
              readable ? PersonalityIndexOf(value) : -1);

    SetEnabled(g_personalityApply, readable);
}

void OnApplyPersonality(MyGUI::Widget* /*sender*/)
{
    Character* character = SelectedCharacter();

    int oldValue = 0;
    PersonalityResult result = ReadPersonality(character, &oldValue);
    if (result != PERSONALITY_OK)
    {
        SetStatus(PersonalityFailureText(result));
        return;
    }

    const int index = ComboSelection(g_personalityCombo);
    if (index < 0)
    {
        SetStatus(TextOf("personality.noChoice", "Pick a personality first."));
        return;
    }

    const int newValue = PersonalityValueAt(index);
    result = WritePersonality(character, newValue);
    if (result != PERSONALITY_OK)
    {
        SetStatus(PersonalityFailureText(result));
        return;
    }

    LogDebug(CharacterName(character) + ": personality changed " +
             PersonalityLabel(oldValue) + " -> " + PersonalityLabel(newValue) + ".");

    RefreshPersonality();
    SetStatus(TextOf("personality.changed",
                     "Personality changed. Kenshi will save it normally."));
}

void Flush()
{
    if (!g_dirty)
    {
        return;
    }
    if (SaveStore())
    {
        g_dirty = false;
    }
}

// ---------------------------------------------------------------------------
// Page six: uniform colours with a live preview, and the portrait.
//
// Every colour change is previewed on the character in the world straight away
// (UniformEngine::PreviewColours) but nothing touches the disk until Save.
// Cancel, closing the window, or selecting someone else reverts the armour to
// what was last saved.
// ---------------------------------------------------------------------------
struct ColourBlock
{
    MyGUI::Widget*  border;
    MyGUI::Widget*  fill;
    MyGUI::EditBox* hex;
    MyGUI::TextBox* readout;
    MyGUI::TextBox* preset;
    MyGUI::Button*  btn[8]; // -R +R -G +G -B +B prev next
    OutlineButton   btnOutline[8];

    ColourBlock() : border(NULL), fill(NULL), hex(NULL), readout(NULL), preset(NULL)
    {
        for (int i = 0; i < 8; ++i) { btn[i] = NULL; }
    }
};

ColourBlock       g_ucPrimary;
ColourBlock       g_ucSecondary;
MyGUI::ImageBox*  g_ucPortrait = NULL;
MyGUI::TextBox*   g_ucName = NULL;
OutlineButton     g_ucEditPortraitBtn;
MyGUI::Button*    g_ucEditPortrait = NULL;
OutlineButton     g_ucVanillaBtn;
MyGUI::Button*    g_ucVanilla = NULL;
OutlineButton     g_ucSaveBtn;
MyGUI::Button*    g_ucSave = NULL;
OutlineButton     g_ucCancelBtn;
MyGUI::Button*    g_ucCancel = NULL;
Character*        g_ucChar = NULL;
bool              g_ucUpdating = false;
bool              g_ucDirty = false;
int               g_ucFrame = 0;

MyGUI::delegates::IDelegate1<MyGUI::Widget*>* g_ucVanillaDelegate = NULL;
MyGUI::delegates::IDelegate1<MyGUI::Widget*>* g_ucSaveDelegate = NULL;
MyGUI::delegates::IDelegate1<MyGUI::Widget*>* g_ucCancelDelegate = NULL;
MyGUI::delegates::IDelegate1<MyGUI::Widget*>* g_ucEditPortraitDelegate = NULL;
MyGUI::delegates::IDelegate1<MyGUI::EditBox*>* g_ucPrimaryHexDelegate = NULL;
MyGUI::delegates::IDelegate1<MyGUI::EditBox*>* g_ucSecondaryHexDelegate = NULL;
MyGUI::delegates::IDelegate1<MyGUI::Widget*>* g_ucStepDelegates[2][8] = { { NULL } };

const char* const kUcDefault = "191919";

void UcShowBlock(ColourBlock& block, const std::string& hex)
{
    g_ucUpdating = true;
    if (block.hex != NULL && GetInputText(block.hex) != hex)
    {
        SetInputText(block.hex, hex);
    }
    if (block.readout != NULL)
    {
        SetText(block.readout, ColourAdjustRules::Readout(hex));
    }
    if (block.preset != NULL)
    {
        SetText(block.preset, ColourAdjustRules::PresetName(hex));
    }
    if (block.fill != NULL)
    {
        UniformColour::Rgb rgb = { 0, 0, 0 };
        if (UniformColour::ParseHex(hex, &rgb))
        {
            float r = 0.f, g = 0.f, b = 0.f;
            UniformColour::ToUnit(rgb, &r, &g, &b);
            block.fill->setColour(MyGUI::Colour(r, g, b, 1.0f));
            block.fill->setVisible(true);
        }
        else
        {
            block.fill->setVisible(false);
        }
    }
    g_ucUpdating = false;
}

std::string UcHexOf(ColourBlock& block)
{
    std::string norm;
    if (block.hex != NULL && UniformColour::NormalizeText(GetInputText(block.hex), &norm))
    {
        return norm;
    }
    return kUcDefault;
}

// Pushes the current boxes onto the character in the world. No disk write.
void UcPreview()
{
    if (g_ucChar == NULL) { return; }
    UniformEngine::PreviewColours(g_ucChar, UcHexOf(g_ucPrimary), UcHexOf(g_ucSecondary));
    g_ucDirty = true;
    SetStatus(TextOf("status.uniformPending", "Colour preview active -- changes pending."), kAmber);
}

// Fills both blocks with what is saved for this character (or the default).
void UcLoadSaved()
{
    std::string p = kUcDefault;
    std::string s = kUcDefault;
    std::string uid;
    if (g_ucChar != NULL && CharacterIdentity::GetOrCreate(g_ucChar, uid))
    {
        std::string sp, ss;
        if (UniformStore::Get(uid, &sp, &ss)) { p = sp; s = ss; }
    }
    UcShowBlock(g_ucPrimary, p);
    UcShowBlock(g_ucSecondary, s);
}

// Reverts any unsaved preview on the character being edited.
void UcDiscard()
{
    if (g_ucChar != NULL && g_ucDirty)
    {
        UniformEngine::CancelPreview(g_ucChar);
        // CancelPreview ends the preview session; open a fresh one.
        UniformEngine::BeginPreview(g_ucChar);
    }
    g_ucDirty = false;
}

// Switches the page to another character, discarding the previous one's preview.
void UcFollow(Character* selected)
{
    if (selected == g_ucChar) { return; }

    UcDiscard();
    g_ucChar = selected;
    g_ucDirty = false;

    if (g_ucChar != NULL)
    {
        UniformEngine::BeginPreview(g_ucChar);
        if (g_ucName != NULL) { SetText(g_ucName, CharacterName(g_ucChar)); }
        PortraitEditor::Decorate(g_ucChar, g_ucPortrait);
    }
    else if (g_ucName != NULL)
    {
        SetText(g_ucName, "No character selected. Click one in the world.");
    }
    UcLoadSaved();
}

void OnUcStep(MyGUI::Widget* sender)
{
    ColourBlock* blocks[2] = { &g_ucPrimary, &g_ucSecondary };
    for (int b = 0; b < 2; ++b)
    {
        for (int i = 0; i < 8; ++i)
        {
            if (blocks[b]->btn[i] != sender) { continue; }

            const std::string current = UcHexOf(*blocks[b]);
            std::string next;
            if (i < 6)
            {
                next = ColourAdjustRules::StepChannel(current, i / 2, (i % 2 == 0) ? -15 : 15);
            }
            else
            {
                next = ColourAdjustRules::CyclePreset(current, (i == 6) ? -1 : 1);
            }
            UcShowBlock(*blocks[b], next);
            UcPreview();
            return;
        }
    }
}

void OnUcHexChanged(MyGUI::EditBox* edit)
{
    if (g_ucUpdating || edit == NULL) { return; }

    ColourBlock* block = (edit == g_ucPrimary.hex) ? &g_ucPrimary
                       : (edit == g_ucSecondary.hex) ? &g_ucSecondary : NULL;
    if (block == NULL) { return; }

    std::string norm;
    if (UniformColour::NormalizeText(GetInputText(edit), &norm))
    {
        UcShowBlock(*block, norm);
        UcPreview();
    }
}

void OnUcSave(MyGUI::Widget* /*sender*/)
{
    if (g_ucChar == NULL) { return; }
    UniformEngine::SaveColours(g_ucChar, UcHexOf(g_ucPrimary), UcHexOf(g_ucSecondary));
    g_ucDirty = false;
    SetStatus(TextOf("status.uniformSaved", "Uniform colours saved."), kReady);
}

void OnUcCancel(MyGUI::Widget* /*sender*/)
{
    UcDiscard();
    UcLoadSaved();
    SetStatus(TextOf("status.uniformCancelled", "Unsaved colour changes discarded."), kDanger);
}

void OnUcVanilla(MyGUI::Widget* /*sender*/)
{
    if (g_ucChar == NULL) { return; }
    UniformEngine::RestoreVanilla(g_ucChar);
    UniformEngine::BeginPreview(g_ucChar);
    g_ucDirty = false;
    UcShowBlock(g_ucPrimary, kUcDefault);
    UcShowBlock(g_ucSecondary, kUcDefault);
    SetStatus(TextOf("status.uniformVanilla", "Back to the original armour colours."), kMuted);
}

void OnUcEditPortrait(MyGUI::Widget* /*sender*/)
{
    if (g_ucChar == NULL)
    {
        SetStatus(TextOf("appearance.noSelection",
                         "Select a character before opening the editor."));
        return;
    }
    PortraitEditor::Open(g_ucChar);
}

void UcBuildBlock(MyGUI::Widget* page, int x, int y, int width, int index,
                  const char* title, ColourBlock& block)
{
    char name[48] = { 0 };

    // Section card behind this colour block
    MakeCard(page, x - 6, y - 6, width + 12, kRow * 4 + kTightGap * 4 + 14, kSumi, kLine);

    // Swatch: border and fill created as siblings on page so fill always renders on top.
    block.border = page->createWidget<MyGUI::Widget>(
        "WhiteSkin", MyGUI::IntCoord(x, y, 64, 30), MyGUI::Align::Default);
    if (block.border != NULL)
    {
        block.border->setNeedMouseFocus(false);
        block.border->setColour(kGoldDim);
    }
    block.fill = page->createWidget<MyGUI::Widget>(
        "WhiteSkin", MyGUI::IntCoord(x + 2, y + 2, 60, 26), MyGUI::Align::Default);
    if (block.fill != NULL)
    {
        block.fill->setNeedMouseFocus(false);
    }

    _snprintf_s(name, sizeof(name), _TRUNCATE, "TE_UcTitle%d", index);
    MyGUI::TextBox* titleLabel = MakeLabel(page, name, x + 76, y + 2, 200, kRow);
    SetText(titleLabel, title);
    SetTextColour(titleLabel, kBrass);

    _snprintf_s(name, sizeof(name), _TRUNCATE, "TE_UcHex%d", index);
    block.hex = MakeInput(page, name, x + width - 110, y, 110, kRow);

    const char* const channels[3] = { "R", "G", "B" };
    int rowY = y + kRow + kTightGap;
    for (int ch = 0; ch < 3; ++ch)
    {
        _snprintf_s(name, sizeof(name), _TRUNCATE, "TE_UcCh%d_%d", index, ch);
        MyGUI::TextBox* chLabel = MakeLabel(page, name, x, rowY, 24, kRow);
        SetText(chLabel, channels[ch]);
        SetTextColour(chLabel, kMuted);

        for (int k = 0; k < 2; ++k)
        {
            _snprintf_s(name, sizeof(name), _TRUNCATE, "TE_UcBtn%d_%d_%d", index, ch, k);
            int btnIdx = ch * 2 + k;
            BuildOutlineButton(page, x + 30 + k * 66, rowY, 62, kRow, name,
                               (k == 0) ? "-15" : "+15", kLine, &block.btnOutline[btnIdx]);
            block.btn[btnIdx] = block.btnOutline[btnIdx].button;
            SetOutlineButtonColour(&block.btnOutline[btnIdx], kLine, kTextPrimary);
        }
        rowY += kRow + kTightGap;
    }

    // Preset cycler on the right of the channel rows.
    const int cycX = x + 180;
    const int cycW = width - 180;
    const int cycY = y + kRow + kTightGap;

    _snprintf_s(name, sizeof(name), _TRUNCATE, "TE_UcPrev%d", index);
    BuildOutlineButton(page, cycX, cycY, 40, kRow, name, "<", kBrass, &block.btnOutline[6]);
    block.btn[6] = block.btnOutline[6].button;

    _snprintf_s(name, sizeof(name), _TRUNCATE, "TE_UcPreset%d", index);
    block.preset = MakeLabel(page, name, cycX + 48, cycY + 4, cycW - 96, kRow);
    SetTextColour(block.preset, kAmber);

    _snprintf_s(name, sizeof(name), _TRUNCATE, "TE_UcNext%d", index);
    BuildOutlineButton(page, cycX + cycW - 40, cycY, 40, kRow, name, ">", kBrass, &block.btnOutline[7]);
    block.btn[7] = block.btnOutline[7].button;

    _snprintf_s(name, sizeof(name), _TRUNCATE, "TE_UcRead%d", index);
    block.readout = MakeLabel(page, name, cycX, cycY + kRow + kGap, cycW, kRow);
    SetTextColour(block.readout, kMuted);
}

void BuildUniformPage(MyGUI::Widget* page, int width, int height)
{
    const int half = (width - kMargin * 3) / 2;
    int y = kMargin;

    g_pageHelp6 = MakeStaticText(page, "TE_PageHelp6", kMargin, y,
                                 width - kMargin * 2, kRow * 2);
    SetBodyText(g_pageHelp6, TextOf("help.uniform",
        "Live preview of uniform armour colours and character portrait. Adjust colours or "
        "open the portrait camera editor. Changes to colours are live and only saved when you "
        "click Save."));
    SetTextColour(g_pageHelp6, kMuted);
    y += kRow * 2 + kGap;

    // LEFT: the portrait preview and camera editor jump.
    const int leftTop = y;
    const int colHeight = height - leftTop - kMargin;
    MakeCard(page, kMargin - 6, leftTop - 6, half + 12, colHeight + 12, kBgDark, kLine);

    MakeHeadingWithRule(page, kMargin, leftTop, half, kHeadingHeight,
                        TextOf("heading.portrait", "PORTRAIT PREVIEW"), "TE_UcPortraitHead");
    int leftY = leftTop + kHeadingHeight + kGap;

    const int portraitSize = 256;
    const int portraitLeft = kMargin + (half - portraitSize) / 2;

    MakeFrame(page, portraitLeft - 1, leftY - 1, portraitSize + 2, portraitSize + 2, 1, kGoldDim);
    MakeFill(page, portraitLeft, leftY, portraitSize, portraitSize, kBgDark);
    g_ucPortrait = page->createWidget<MyGUI::ImageBox>(
        "ImageBox", MyGUI::IntCoord(portraitLeft, leftY, portraitSize, portraitSize),
        MyGUI::Align::Default);
    if (g_ucPortrait != NULL) { g_ucPortrait->setNeedMouseFocus(false); }
    leftY += portraitSize + kGap;

    g_ucName = MakeLabel(page, "TE_UcName", kMargin, leftY, half, kRow);
    SetTextColour(g_ucName, kBrass);
    leftY += kRow + kTightGap;

    BuildOutlineButton(page, portraitLeft, leftY, portraitSize, kButtonHeight,
                       "TE_UcEditPortrait", TextOf("appearance.editPortrait", "Edit portrait camera"),
                       kBrass, &g_ucEditPortraitBtn);
    g_ucEditPortrait = g_ucEditPortraitBtn.button;

    // RIGHT: the two colour blocks and the action buttons.
    const int rightLeft = kMargin * 2 + half;
    int rightY = leftTop;

    MakeCard(page, rightLeft - 6, leftTop - 6, half + 12, colHeight + 12, kBgDark, kLine);

    MakeHeadingWithRule(page, rightLeft, rightY, half, kHeadingHeight,
                        TextOf("heading.uniform", "UNIFORM COLOURS  (live preview)"), "TE_UcHead");
    rightY += kHeadingHeight + kGap;

    UcBuildBlock(page, rightLeft, rightY, half, 0,
                 TextOf("label.primaryColour", "PRIMARY COLOUR").c_str(), g_ucPrimary);
    rightY += kRow * 4 + kTightGap * 4 + kGap * 2;

    UcBuildBlock(page, rightLeft, rightY, half, 1,
                 TextOf("label.secondaryColour", "SECONDARY COLOUR").c_str(), g_ucSecondary);
    rightY += kRow * 4 + kTightGap * 4 + kGap * 2;

    const int buttonW = (half - kGap * 2) / 3;
    BuildOutlineButton(page, rightLeft, rightY, buttonW, kButtonHeight,
                       "TE_UcVanilla", TextOf("button.vanilla", "Vanilla"), kMuted, &g_ucVanillaBtn);
    g_ucVanilla = g_ucVanillaBtn.button;
    SetOutlineButtonColour(&g_ucVanillaBtn, kLine, kMuted);

    BuildOutlineButton(page, rightLeft + buttonW + kGap, rightY, buttonW, kButtonHeight,
                       "TE_UcSave", TextOf("button.save", "Save"), kBrass, &g_ucSaveBtn);
    g_ucSave = g_ucSaveBtn.button;

    BuildOutlineButton(page, rightLeft + (buttonW + kGap) * 2, rightY, buttonW, kButtonHeight,
                       "TE_UcCancel", TextOf("button.cancel", "Cancel"), kDanger, &g_ucCancelBtn);
    g_ucCancel = g_ucCancelBtn.button;

    UcShowBlock(g_ucPrimary, kUcDefault);
    UcShowBlock(g_ucSecondary, kUcDefault);
    (void)height;
}

void Close()
{
    g_open = false;
    SetVisible(g_root, false);

    // Closing the window is the same as pressing Cancel on the uniform page.
    UcDiscard();

    if (GetConfig().saveOnClose)
    {
        Flush();
    }
}

// CharTweak 2's "Edit Appearance": only the player's own characters, and this
// window closes first so the game's editor is not drawn underneath it.
void OnEditAppearance(MyGUI::Widget* /*sender*/)
{
    Character* character = SelectedCharacter();
    const AppearanceResult allowed = CanEditAppearance(character);
    if (allowed == APPEARANCE_NO_CHARACTER)
    {
        SetStatus(TextOf("appearance.noSelection",
                         "Select a character before opening the editor."));
        return;
    }
    if (allowed == APPEARANCE_NOT_PLAYER)
    {
        SetStatus(TextOf("appearance.notPlayer",
                         "Appearance editor only supports player characters."));
        LogError("blocked appearance editor for non-player character");
        return;
    }
    if (allowed != APPEARANCE_OK)
    {
        SetStatus(TextOf("appearance.failed", "The appearance editor could not be opened."));
        return;
    }

    Close();
    if (OpenAppearanceEditor(character) != APPEARANCE_OK)
    {
        LogError("the appearance editor could not be opened for " + CharacterName(character));
        return;
    }
    LogDebug("appearance editor opened for " + CharacterName(character));
}

void OnWindowButton(MyGUI::Window* /*sender*/, const std::string& button)
{
    // MyGUI names title-bar buttons by their skin; "close" is the cross.
    if (button == "close")
    {
        Close();
    }
}

// Binding a control that was never created cost a whole session of debugging.
//
// BindEvents runs under one __except, so the first null dereference in it
// unwinds the entire function: every control below that line is left with no
// handler, and the catch says nothing. The window then looks frozen except for
// the few controls bound above the fault, which reads as a mouse or z-order
// problem and is not one.
//
// The guard has to be its own statement, before the arrow: writing the event
// as an argument would dereference the pointer at the call site, which is the
// fault itself. Hence a macro rather than a function.
//
// The logging lives out of line because __try may not share a frame with
// anything that needs unwinding, and building the message is a std::string.
void LogMissingWidget(const char* what)
{
    LogError(std::string("No ") + what + " widget to bind: it was never "
             "created. Every control after it would have stayed dead.");
}

void LogBindFaulted()
{
    LogError("Binding the window's events faulted partway through; the "
             "controls below that point will not respond.");
}

#define TE_BIND(what, widget, event, handler)                                  \
    do {                                                                       \
        if ((widget) == NULL)                                                  \
        {                                                                      \
            LogMissingWidget(what);                                            \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            (widget)->event += (handler);                                      \
        }                                                                      \
    } while (0)

void BindEvents()
{
    TE_TRY
    {
        g_closeDelegate = MyGUI::newDelegate(&OnWindowButton);
        TE_BIND("window", g_root, eventWindowButtonPressed, g_closeDelegate);

        g_raceDelegate = MyGUI::newDelegate(&OnRaceChanged);
        TE_BIND("race combo", g_raceCombo, eventComboAccept, g_raceDelegate);

        g_statComboDelegate = MyGUI::newDelegate(&OnStatComboChanged);
        TE_BIND("stat combo", g_statCombo, eventComboAccept, g_statComboDelegate);

        g_bonusAddDelegate = MyGUI::newDelegate(&OnAddBonus);
        TE_BIND("add bonus", g_bonusAdd, eventMouseButtonClick, g_bonusAddDelegate);

        g_bonusDelDelegate = MyGUI::newDelegate(&OnRemoveBonus);
        TE_BIND("remove bonus", g_bonusDel, eventMouseButtonClick, g_bonusDelDelegate);

        g_revertDelegate = MyGUI::newDelegate(&OnRevert);
        TE_BIND("revert race", g_revert, eventMouseButtonClick, g_revertDelegate);

        g_statApplyDelegate = MyGUI::newDelegate(&OnApplyStat);
        TE_BIND("apply stat", g_statApply, eventMouseButtonClick, g_statApplyDelegate);

        g_copyDelegate = MyGUI::newDelegate(&OnCopyBonuses);
        TE_BIND("copy bonuses", g_copyButton, eventMouseButtonClick, g_copyDelegate);

        g_filterDelegate = MyGUI::newDelegate(&OnFilterChanged);
        TE_BIND("race search", g_filterGo, eventMouseButtonClick, g_filterDelegate);

        g_copyTraitsDelegate = MyGUI::newDelegate(&OnCopyTraitsClicked);
        TE_BIND("copy traits", g_copyTraits, eventMouseButtonClick, g_copyTraitsDelegate);

        g_jumpRaceDelegate = MyGUI::newDelegate(&OnJumpToRace);
        TE_BIND("jump to race", g_jumpRace, eventMouseButtonClick, g_jumpRaceDelegate);

        g_editAppearanceDelegate = MyGUI::newDelegate(&OnEditAppearance);
        TE_BIND("edit appearance", g_editAppearance, eventMouseButtonClick,
                g_editAppearanceDelegate);

        g_bonusSearchDelegate = MyGUI::newDelegate(&OnBonusSearch);
        TE_BIND("bonus search", g_bonusSearchGo, eventMouseButtonClick,
                g_bonusSearchDelegate);

        g_statSearchDelegate = MyGUI::newDelegate(&OnStatSearch);
        TE_BIND("stat search", g_statSearchGo, eventMouseButtonClick,
                g_statSearchDelegate);

        g_partApplyDelegate = MyGUI::newDelegate(&OnApplyPart);
        TE_BIND("apply part", g_partApply, eventMouseButtonClick, g_partApplyDelegate);

        g_bloodApplyDelegate = MyGUI::newDelegate(&OnApplyBlood);
        TE_BIND("apply blood", g_bloodApply, eventMouseButtonClick, g_bloodApplyDelegate);

        g_skillListDelegate = MyGUI::newDelegate(&OnSkillListSelected);
        TE_BIND("skill list", g_skillList, eventListChangePosition,
                g_skillListDelegate);

        g_raceBonusDelegate = MyGUI::newDelegate(&OnRaceBonusSelected);
        TE_BIND("race bonus list", g_raceBonusList, eventListChangePosition,
                g_raceBonusDelegate);

        g_partListDelegate = MyGUI::newDelegate(&OnPartSelected);
        TE_BIND("part list", g_partList, eventListChangePosition, g_partListDelegate);

        g_realOnlyDelegate = MyGUI::newDelegate(&OnRealOnlyClicked);
        TE_BIND("real races only", g_realOnly, eventMouseButtonClick,
                g_realOnlyDelegate);

        g_hitApplyDelegate = MyGUI::newDelegate(&OnApplyHitChance);
        TE_BIND("apply hit chance", g_hitApply, eventMouseButtonClick,
                g_hitApplyDelegate);

        g_personalityApplyDelegate = MyGUI::newDelegate(&OnApplyPersonality);
        TE_BIND("apply personality", g_personalityApply, eventMouseButtonClick,
                g_personalityApplyDelegate);

        for (int i = 0; i < kPageCount; ++i)
        {
            if (g_tabButtons[i] != NULL)
            {
                g_tabDelegates[i] = MyGUI::newDelegate(&OnTabClicked);
                g_tabButtons[i]->eventMouseButtonClick += g_tabDelegates[i];
            }
        }

        // One delegate per tick box. They all land in the same handler, which
        // works out which box was clicked from the sender.
        for (int i = 0; i < kFlagRowCount; ++i)
        {
            if (g_flagBoxes[i] != NULL)
            {
                g_flagBoxDelegates[i] = MyGUI::newDelegate(&OnFlagBoxClicked);
                g_flagBoxes[i]->eventMouseButtonClick += g_flagBoxDelegates[i];
            }
        }

        // The copy dropdown shares the race filter, so its own search box
        // narrows only that list and leaves the race being edited alone.
        g_copySearchDelegate = MyGUI::newDelegate(&OnCopySearch);
        TE_BIND("copy search", g_copySearchGo, eventMouseButtonClick,
                g_copySearchDelegate);

        g_copyComboDelegate = MyGUI::newDelegate(&OnCopySourceChanged);
        TE_BIND("copy source combo", g_copyCombo, eventComboAccept,
                g_copyComboDelegate);

        if (g_bonusBar != NULL)
        {
            g_bonusBarDelegate = MyGUI::newDelegate(&OnBonusBarMoved);
            g_bonusBar->eventScrollChangePosition += g_bonusBarDelegate;
        }
        if (g_statBar != NULL)
        {
            g_statBarDelegate = MyGUI::newDelegate(&OnStatBarMoved);
            g_statBar->eventScrollChangePosition += g_statBarDelegate;
        }

        g_ucVanillaDelegate = MyGUI::newDelegate(&OnUcVanilla);
        TE_BIND("uc vanilla", g_ucVanilla, eventMouseButtonClick, g_ucVanillaDelegate);

        g_ucSaveDelegate = MyGUI::newDelegate(&OnUcSave);
        TE_BIND("uc save", g_ucSave, eventMouseButtonClick, g_ucSaveDelegate);

        g_ucCancelDelegate = MyGUI::newDelegate(&OnUcCancel);
        TE_BIND("uc cancel", g_ucCancel, eventMouseButtonClick, g_ucCancelDelegate);

        g_ucEditPortraitDelegate = MyGUI::newDelegate(&OnUcEditPortrait);
        TE_BIND("uc edit portrait", g_ucEditPortrait, eventMouseButtonClick, g_ucEditPortraitDelegate);

        if (g_ucPrimary.hex != NULL)
        {
            g_ucPrimaryHexDelegate = MyGUI::newDelegate(&OnUcHexChanged);
            g_ucPrimary.hex->eventEditTextChange += g_ucPrimaryHexDelegate;
        }
        if (g_ucSecondary.hex != NULL)
        {
            g_ucSecondaryHexDelegate = MyGUI::newDelegate(&OnUcHexChanged);
            g_ucSecondary.hex->eventEditTextChange += g_ucSecondaryHexDelegate;
        }

        ColourBlock* ucBlocks[2] = { &g_ucPrimary, &g_ucSecondary };
        for (int b = 0; b < 2; ++b)
        {
            for (int i = 0; i < 8; ++i)
            {
                if (ucBlocks[b]->btn[i] != NULL)
                {
                    g_ucStepDelegates[b][i] = MyGUI::newDelegate(&OnUcStep);
                    ucBlocks[b]->btn[i]->eventMouseButtonClick += g_ucStepDelegates[b][i];
                }
            }
        }
    }
    TE_CATCH
    {
        // Reaching here means the window is half wired and looks frozen. The
        // silent version of this handler is what made the null skill list read
        // as a mouse problem for an entire session.
        LogBindFaulted();
    }
}

void Unbind()
{
    TE_TRY
    {
        if (g_root != NULL && g_closeDelegate != NULL)
        { g_root->eventWindowButtonPressed -= g_closeDelegate; }
        if (g_raceCombo != NULL && g_raceDelegate != NULL)
        { g_raceCombo->eventComboAccept -= g_raceDelegate; }
        if (g_statCombo != NULL && g_statComboDelegate != NULL)
        { g_statCombo->eventComboAccept -= g_statComboDelegate; }
        if (g_bonusAdd != NULL && g_bonusAddDelegate != NULL)
        { g_bonusAdd->eventMouseButtonClick -= g_bonusAddDelegate; }
        if (g_bonusDel != NULL && g_bonusDelDelegate != NULL)
        { g_bonusDel->eventMouseButtonClick -= g_bonusDelDelegate; }
        if (g_revert != NULL && g_revertDelegate != NULL)
        { g_revert->eventMouseButtonClick -= g_revertDelegate; }
        if (g_statApply != NULL && g_statApplyDelegate != NULL)
        { g_statApply->eventMouseButtonClick -= g_statApplyDelegate; }
        if (g_copyButton != NULL && g_copyDelegate != NULL)
        { g_copyButton->eventMouseButtonClick -= g_copyDelegate; }
        if (g_filterGo != NULL && g_filterDelegate != NULL)
        { g_filterGo->eventMouseButtonClick -= g_filterDelegate; }
        if (g_copyTraits != NULL && g_copyTraitsDelegate != NULL)
        { g_copyTraits->eventMouseButtonClick -= g_copyTraitsDelegate; }
        if (g_jumpRace != NULL && g_jumpRaceDelegate != NULL)
        { g_jumpRace->eventMouseButtonClick -= g_jumpRaceDelegate; }
        if (g_editAppearance != NULL && g_editAppearanceDelegate != NULL)
        { g_editAppearance->eventMouseButtonClick -= g_editAppearanceDelegate; }
        if (g_bonusSearchGo != NULL && g_bonusSearchDelegate != NULL)
        { g_bonusSearchGo->eventMouseButtonClick -= g_bonusSearchDelegate; }
        if (g_statSearchGo != NULL && g_statSearchDelegate != NULL)
        { g_statSearchGo->eventMouseButtonClick -= g_statSearchDelegate; }
        if (g_partApply != NULL && g_partApplyDelegate != NULL)
        { g_partApply->eventMouseButtonClick -= g_partApplyDelegate; }
        if (g_bloodApply != NULL && g_bloodApplyDelegate != NULL)
        { g_bloodApply->eventMouseButtonClick -= g_bloodApplyDelegate; }
        if (g_skillList != NULL && g_skillListDelegate != NULL)
        { g_skillList->eventListChangePosition -= g_skillListDelegate; }
        if (g_raceBonusList != NULL && g_raceBonusDelegate != NULL)
        { g_raceBonusList->eventListChangePosition -= g_raceBonusDelegate; }
        if (g_partList != NULL && g_partListDelegate != NULL)
        { g_partList->eventListChangePosition -= g_partListDelegate; }
        if (g_realOnly != NULL && g_realOnlyDelegate != NULL)
        { g_realOnly->eventMouseButtonClick -= g_realOnlyDelegate; }
        if (g_hitApply != NULL && g_hitApplyDelegate != NULL)
        { g_hitApply->eventMouseButtonClick -= g_hitApplyDelegate; }
        if (g_personalityApply != NULL && g_personalityApplyDelegate != NULL)
        { g_personalityApply->eventMouseButtonClick -= g_personalityApplyDelegate; }
        for (int i = 0; i < kFlagRowCount; ++i)
        {
            if (g_flagBoxes[i] != NULL && g_flagBoxDelegates[i] != NULL)
            {
                g_flagBoxes[i]->eventMouseButtonClick -= g_flagBoxDelegates[i];
            }
        }
        for (int i = 0; i < kPageCount; ++i)
        {
            if (g_tabButtons[i] != NULL && g_tabDelegates[i] != NULL)
            {
                g_tabButtons[i]->eventMouseButtonClick -= g_tabDelegates[i];
            }
        }
        if (g_copySearchGo != NULL && g_copySearchDelegate != NULL)
        { g_copySearchGo->eventMouseButtonClick -= g_copySearchDelegate; }
        if (g_copyCombo != NULL && g_copyComboDelegate != NULL)
        { g_copyCombo->eventComboAccept -= g_copyComboDelegate; }
        if (g_bonusBar != NULL && g_bonusBarDelegate != NULL)
        { g_bonusBar->eventScrollChangePosition -= g_bonusBarDelegate; }
        if (g_statBar != NULL && g_statBarDelegate != NULL)
        { g_statBar->eventScrollChangePosition -= g_statBarDelegate; }
        if (g_ucVanilla != NULL && g_ucVanillaDelegate != NULL)
        { g_ucVanilla->eventMouseButtonClick -= g_ucVanillaDelegate; }
        if (g_ucSave != NULL && g_ucSaveDelegate != NULL)
        { g_ucSave->eventMouseButtonClick -= g_ucSaveDelegate; }
        if (g_ucCancel != NULL && g_ucCancelDelegate != NULL)
        { g_ucCancel->eventMouseButtonClick -= g_ucCancelDelegate; }
        if (g_ucEditPortrait != NULL && g_ucEditPortraitDelegate != NULL)
        { g_ucEditPortrait->eventMouseButtonClick -= g_ucEditPortraitDelegate; }
        if (g_ucPrimary.hex != NULL && g_ucPrimaryHexDelegate != NULL)
        { g_ucPrimary.hex->eventEditTextChange -= g_ucPrimaryHexDelegate; }
        if (g_ucSecondary.hex != NULL && g_ucSecondaryHexDelegate != NULL)
        { g_ucSecondary.hex->eventEditTextChange -= g_ucSecondaryHexDelegate; }
        ColourBlock* ucUnbindBlocks[2] = { &g_ucPrimary, &g_ucSecondary };
        for (int b = 0; b < 2; ++b)
        {
            for (int i = 0; i < 8; ++i)
            {
                if (ucUnbindBlocks[b]->btn[i] != NULL && g_ucStepDelegates[b][i] != NULL)
                { ucUnbindBlocks[b]->btn[i]->eventMouseButtonClick -= g_ucStepDelegates[b][i]; }
            }
        }
    }
    TE_CATCH { }
}

void ChoosePosition(int width, int height, int* outLeft, int* outTop)
{
    int viewWidth = 0;
    int viewHeight = 0;
    ViewSize(&viewWidth, &viewHeight);

    *outLeft = (viewWidth > width) ? (viewWidth - width) / 2 : 40;
    *outTop = (viewHeight > height) ? (viewHeight - height) / 2 : 40;
    if (*outTop < 30)
    {
        *outTop = 30;
    }
}

// Lays the two panels out side by side, each taking half the client width.
// Computed rather than hardcoded, so a window resized through config.json still
// draws correctly.
// Left panel: the race and its shared bonuses.
//
// The list takes whatever vertical room the fixed controls below it leave, so
// the window can grow through config.json and the list grows with it rather
// than leaving a gap.
// One labelled search box above a dropdown, which is how every select in this
// window is built.
//
// The race lists run to nearly two thousand entries and the skill lists to
// thirty-six; a search box earns its place on the first and costs almost
// nothing on the second, and having them all behave the same way means the
// player learns one control rather than three.
struct SearchRow
{
    MyGUI::TextBox*  label;
    MyGUI::EditBox*  search;
    MyGUI::Button*   go;
    MyGUI::ComboBox* combo;

    SearchRow() : label(NULL), search(NULL), go(NULL), combo(NULL) {}
};

// Lays out label, search box + button, and the dropdown, and reports the y the
// next control should start at.
int BuildSearchRow(SearchRow* row, const char* labelName, const char* searchName,
                   const char* goName, const char* comboName,
                   int left, int top, int width)
{
    int y = top;

    row->label = MakeLabel(g_client, labelName, left, y, width, kLabelHeight);
    y += kLabelHeight + kTightGap;

    const int goWidth = 78;
    row->search = MakeInput(g_client, searchName, left, y, width - goWidth - kTightGap, kRow);
    row->go = MakeButton(g_client, goName, left + width - goWidth, y, goWidth, kRow);
    y += kRow + kTightGap;

    row->combo = MakeCombo(g_client, comboName, left, y, width, kRow);
    y += kRow + kGap;

    return y;
}

// Column one: choosing a race and seeing what it carries.
// Every page that edits a race carries its own race picker: switching back to
// another tab to change which race you are editing is a step nobody should
// have to take twice.
int BuildRacePicker(MyGUI::Widget* page, int left, int top, int width,
                    const char* labelName, const char* searchName,
                    const char* goName, const char* comboName,
                    MyGUI::TextBox** label, MyGUI::EditBox** search,
                    MyGUI::Button** go, MyGUI::ComboBox** combo)
{
    int y = top;

    *label = MakeLabel(page, labelName, left, y, width, kLabelHeight);
    y += kLabelHeight + kTightGap;

    const int goWidth = 80;
    *search = MakeInput(page, searchName, left, y, width - goWidth - kTightGap, kRow);

    OutlineButton* goBtnPtr = (strcmp(goName, kCopySearchGoName) == 0) ? &g_copySearchGoBtn
                            : (strcmp(goName, kFilterGoName) == 0) ? &g_filterGoBtn
                            : &g_bonusSearchGoBtn;
    BuildOutlineButton(page, left + width - goWidth, y, goWidth, kRow,
                       goName, "Search", kBrass, goBtnPtr);
    *go = goBtnPtr->button;
    y += kRow + kTightGap;

    *combo = MakeCombo(page, comboName, left, y, width, kRow);
    y += kRow + kGap;

    return y;
}

// Page one: choose a race and read everything the engine knows about it.
void BuildOverviewPage(MyGUI::Widget* page, int width, int height)
{
    const int half = (width - kMargin * 3) / 2;
    int y = kMargin;

    g_pageHelp1 = MakeStaticText(page, kPageHelp1Name, kMargin, y,
                                 width - kMargin * 2, kRow * 2);
    y += kRow * 2 + kGap;

    const int colHeight = height - y - kMargin;
    MakeCard(page, kMargin - 6, y - 6, half + 12, colHeight + 12, kBgDark, kLine);

    // LEFT: the race being copied FROM, and everything it carries.
    g_copyHead = MakeHeadingWithRule(page, kMargin, y, half, kHeadingHeight,
                                     TextOf("copy.head", "SOURCE RACE (BONUSES TO COPY)"), kCopyHeadName);
    int leftY = y + kHeadingHeight + kGap;

    leftY = BuildRacePicker(page, kMargin, leftY, half,
                            kCopyLabelName, kCopySearchName, kCopySearchGoName,
                            kCopyComboName, &g_copyLabel, &g_copySearch,
                            &g_copySearchGo, &g_copyCombo);

    g_copyTraits = MakeTickBox(page, kCopyTraitsName, kMargin, leftY, 26, 26);
    g_copyTraitsLabel = MakeLabel(page, kCopyTraitsLabelName, kMargin + 32, leftY,
                                  half - 32, 26);
    leftY += 26 + kGap;

    g_detailsHead = MakeHeadingWithRule(page, kMargin, leftY, half, kHeadingHeight,
                                        TextOf("details.head", "TARGET RACE DETAILS"), kDetailsHeadName);
    leftY += kHeadingHeight + kGap;

    // RIGHT: the race being changed, laid out to mirror the left.
    const int rightLeft = kMargin * 2 + half;
    MakeCard(page, rightLeft - 6, y - 6, half + 12, colHeight + 12, kBgDark, kLine);

    int rightY = y;
    g_bonusHead = MakeHeadingWithRule(page, rightLeft, rightY, half, kHeadingHeight,
                                      TextOf("bonus.head", "TARGET RACE (RECEIVES BONUSES)"), kBonusHeadName);
    rightY += kHeadingHeight + kGap;

    rightY = BuildRacePicker(page, rightLeft, rightY, half,
                             kRaceLabelName, kFilterName, kFilterGoName,
                             kRaceComboName, &g_raceLabel, &g_filter,
                             &g_filterGo, &g_raceCombo);

    g_realOnly = MakeTickBox(page, kRealOnlyName, rightLeft, rightY, 26, 26);
    g_realOnlyLabel = MakeLabel(page, kRealOnlyLabelName, rightLeft + 32, rightY,
                                half - 32, 26);
    rightY += 26 + kGap;

    g_bonusListHead = MakeHeadingWithRule(page, rightLeft, rightY, half, kHeadingHeight,
                                          TextOf("bonusList.head", "MODIFIED BONUSES ON TARGET"), kBonusListHeadName);
    rightY += kHeadingHeight + kGap;

    const int panelTop = (leftY > rightY) ? leftY : rightY;
    int panelHeight = height - panelTop - kMargin - kButtonHeight - kGap;
    if (panelHeight < kRow * 6)
    {
        panelHeight = kRow * 6;
    }

    g_details = MakeList(page, kDetailsName, kMargin, panelTop,
                         half, panelHeight);
    g_targetDetails = MakeList(page, kTargetDetailsName, rightLeft,
                               panelTop, half, panelHeight);

    const int buttonY = panelTop + panelHeight + kGap;
    BuildOutlineButton(page, kMargin, buttonY, half, kButtonHeight,
                       kCopyButtonName, TextOf("copy.button", "Copy source onto target"),
                       kAmber, &g_copyBtn);
    g_copyButton = g_copyBtn.button;

    BuildOutlineButton(page, rightLeft, buttonY, half, kButtonHeight,
                       kRevertName, TextOf("revert.button", "Revert target race"),
                       kDanger, &g_revertBtn);
    g_revert = g_revertBtn.button;
}

// Page two: adding and removing one bonus at a time.
void BuildBonusPage(MyGUI::Widget* page, int width, int height)
{
    const int half = (width - kMargin * 3) / 2;
    int y = kMargin;

    g_pageHelp2 = MakeStaticText(page, kPageHelp2Name, kMargin, y,
                                 width - kMargin * 2, kRow * 2);
    y += kRow * 2 + kGap;

    const int colHeight = height - y - kMargin;
    MakeCard(page, kMargin - 6, y - 6, half + 12, colHeight + 12, kBgDark, kLine);

    g_editHead = MakeHeadingWithRule(page, kMargin, y, half, kHeadingHeight,
                                     TextOf("edit.head", "EDIT BONUS"), kEditHeadName);
    y += kHeadingHeight + kGap;

    g_bonusLabel = MakeLabel(page, kBonusLabelName, kMargin, y, half, kLabelHeight);
    y += kLabelHeight + kTightGap;

    const int goWidth = 80;
    g_bonusSearch = MakeInput(page, kBonusSearchName, kMargin, y,
                              half - goWidth - kTightGap, kRow);
    BuildOutlineButton(page, kMargin + half - goWidth, y, goWidth, kRow,
                       kBonusSearchGoName, "Search", kBrass, &g_bonusSearchGoBtn);
    g_bonusSearchGo = g_bonusSearchGoBtn.button;
    y += kRow + kTightGap;

    g_bonusStat = MakeCombo(page, kBonusStatName, kMargin, y, half, kRow);
    y += kRow + kGap;

    g_bonusValueLabel = MakeLabel(page, kBonusValueLabelName, kMargin, y,
                                  half, kLabelHeight);
    y += kLabelHeight + kTightGap;

    g_bonusValue = MakeInput(page, kBonusValueName, kMargin, y, half, kRow);
    y += kRow + kTightGap;

    g_bonusBar = MakeScrollBar(page, kBonusBarName, kMargin, y, half, kBarHeight);
    y += kBarHeight + kGap;

    BuildOutlineButton(page, kMargin, y, half, kButtonHeight,
                       kBonusAddName, TextOf("bonus.add", "[+]  Add / update bonus"),
                       kAmber, &g_bonusAddBtn);
    g_bonusAdd = g_bonusAddBtn.button;
    y += kButtonHeight + kTightGap;

    BuildOutlineButton(page, kMargin, y, half, kButtonHeight,
                       kBonusDelName, TextOf("bonus.remove", "[X]  Remove bonus"),
                       kDanger, &g_bonusDelBtn);
    g_bonusDel = g_bonusDelBtn.button;

    // The right half: what this race carries right now.
    const int rightLeft = kMargin * 2 + half;
    int rightY = kMargin + kRow * 2 + kGap;
    MakeCard(page, rightLeft - 6, rightY - 6, half + 12, colHeight + 12, kBgDark, kLine);

    g_raceBonusHead = MakeHeadingWithRule(page, rightLeft, rightY, half, kHeadingHeight,
                                          TextOf("raceBonus.head", "RACE BONUSES"), kRaceBonusHeadName);
    rightY += kHeadingHeight + kGap;

    int bonusListHeight = height - rightY - kMargin;
    if (bonusListHeight < kRow * 4)
    {
        bonusListHeight = kRow * 4;
    }
    g_raceBonusList = MakeList(page, kRaceBonusListName, rightLeft, rightY,
                               half, bonusListHeight);
}

// Page three: the body parts, their health and how often each is hit.
void BuildPartPage(MyGUI::Widget* page, int width, int height)
{
    const int half = (width - kMargin * 3) / 2;
    int y = kMargin;

    g_pageHelp3 = MakeStaticText(page, kPageHelp3Name, kMargin, y,
                                 width - kMargin * 2, kRow * 2);
    y += kRow * 2 + kGap;

    const int colHeight = height - y - kMargin;
    MakeCard(page, kMargin - 6, y - 6, half + 12, colHeight + 12, kBgDark, kLine);

    g_partHead = MakeHeadingWithRule(page, kMargin, y, half, kHeadingHeight,
                                     TextOf("part.head", "BODY PARTS"), kPartHeadName);

    const int listTop = y + kHeadingHeight + kGap;
    int listHeight = height - listTop - kMargin;
    if (listHeight < kRow * 5)
    {
        listHeight = kRow * 5;
    }
    g_partList = MakeList(page, kPartListName, kMargin, listTop, half, listHeight);

    // Right: the two numbers of whichever part is selected.
    const int rightLeft = kMargin * 2 + half;
    int rightY = y;
    MakeCard(page, rightLeft - 6, y - 6, half + 12, colHeight + 12, kBgDark, kLine);

    g_partLabel = MakeLabel(page, kPartLabelName, rightLeft, rightY, half, kLabelHeight);
    rightY += kLabelHeight + kTightGap;

    g_partValue = MakeInput(page, kPartValueName, rightLeft, rightY, half, kRow);
    rightY += kRow + kTightGap;

    BuildOutlineButton(page, rightLeft, rightY, half, kButtonHeight,
                       kPartApplyName, TextOf("part.apply", "Apply part health"),
                       kAmber, &g_partApplyBtn);
    g_partApply = g_partApplyBtn.button;
    rightY += kButtonHeight + kGap * 2;

    g_hitLabel = MakeLabel(page, kHitLabelName, rightLeft, rightY, half, kLabelHeight);
    rightY += kLabelHeight + kTightGap;

    g_hitValue = MakeInput(page, kHitValueName, rightLeft, rightY, half, kRow);
    rightY += kRow + kTightGap;

    BuildOutlineButton(page, rightLeft, rightY, half, kButtonHeight,
                       kHitApplyName, TextOf("hit.apply", "Apply hit chance"),
                       kAmber, &g_hitApplyBtn);
    g_hitApply = g_hitApplyBtn.button;
    rightY += kButtonHeight + kGap * 2;

    g_bloodHead = MakeHeadingWithRule(page, rightLeft, rightY, half, kHeadingHeight,
                                      TextOf("blood.head", "BLOOD MULTIPLIER"), kBloodHeadName);
    rightY += kHeadingHeight + kGap;

    const int bloodHalf = (half - kGap) / 2;

    g_bloodMinLabel = MakeLabel(page, kBloodMinLabelName, rightLeft, rightY,
                                bloodHalf, kLabelHeight);
    g_bloodMaxLabel = MakeLabel(page, kBloodMaxLabelName, rightLeft + bloodHalf + kGap,
                                rightY, bloodHalf, kLabelHeight);
    rightY += kLabelHeight + kTightGap;

    g_bloodMin = MakeInput(page, kBloodMinName, rightLeft, rightY, bloodHalf, kRow);
    g_bloodMax = MakeInput(page, kBloodMaxName, rightLeft + bloodHalf + kGap,
                           rightY, bloodHalf, kRow);
    rightY += kRow + kTightGap;

    BuildOutlineButton(page, rightLeft, rightY, half, kButtonHeight,
                       kBloodApplyName, TextOf("blood.apply", "Apply blood multiplier"),
                       kAmber, &g_bloodApplyBtn);
    g_bloodApply = g_bloodApplyBtn.button;
}

// Page four: what the race is allowed to do, one tick box per switch.
void BuildFlagPage(MyGUI::Widget* page, int width, int /*height*/)
{
    int y = kMargin;

    g_pageHelp4 = MakeStaticText(page, kPageHelp4Name, kMargin, y,
                                 width - kMargin * 2, kRow * 3);
    y += kRow * 3 + kGap;

    const int columnWidth = (width - kMargin * 3) / 2;
    const int perColumn = (kFlagRowCount + 1) / 2;
    const int gridHeight = perColumn * (kRow + kGap) + kGap;

    MakeCard(page, kMargin - 6, y - 6, columnWidth + 12, gridHeight + 12, kBgDark, kLine);
    MakeCard(page, kMargin + columnWidth + kMargin - 6, y - 6, columnWidth + 12, gridHeight + 12, kBgDark, kLine);

    for (int i = 0; i < kFlagRowCount; ++i)
    {
        const int column = (i < perColumn) ? 0 : 1;
        const int row = (i < perColumn) ? i : (i - perColumn);
        const int x = kMargin + column * (columnWidth + kMargin);
        const int rowY = y + row * (kRow + kGap);

        char boxName[64] = { 0 };
        char labelName[64] = { 0 };
        _snprintf_s(boxName, sizeof(boxName), _TRUNCATE, "TE_Flag%d", i);
        _snprintf_s(labelName, sizeof(labelName), _TRUNCATE, "TE_FlagLbl%d", i);

        g_flagBoxes[i] = MakeTickBox(page, boxName, x, rowY, 28, 28);
        g_flagLabels[i] = MakeLabel(page, labelName, x + 34, rowY,
                                    columnWidth - 34, 28);

        SetText(g_flagLabels[i], TextOf(kFlagRows[i].key, kFlagRows[i].label));
        SetTextColour(g_flagLabels[i], kTextPrimary);
    }
}

void BuildPersonalityBlock(MyGUI::Widget* page, int left, int top, int width)
{
    const int headWidth = width / 3;
    g_personalityHead = MakeHeadingWithRule(page, left, top, headWidth, kHeadingHeight,
                                            TextOf("personality.head", "PERSONALITY"), kPersonalityHeadName);
    g_personalityCurrent = MakeLabel(page, kPersonalityCurrentName, left + headWidth,
                                     top, width - headWidth, kHeadingHeight);
    const int y = top + kHeadingHeight + kGap;

    const int applyWidth = 140;
    const int comboWidth = width - applyWidth - kTightGap;

    g_personalityCombo = MakeCombo(page, kPersonalityComboName, left, y, comboWidth, kRow);
    BuildOutlineButton(page, left + comboWidth + kTightGap, y, applyWidth, kRow,
                       kPersonalityApplyName, TextOf("personality.apply", "Apply personality"),
                       kAmber, &g_personalityApplyBtn);
    g_personalityApply = g_personalityApplyBtn.button;
}

// Page five: the selected character, the one page that edits a person rather
// than a race.
void BuildCharacterPage(MyGUI::Widget* page, int width, int height)
{
    const int half = (width - kMargin * 3) / 2;
    int y = kMargin;

    g_pageHelp5 = MakeStaticText(page, kPageHelp5Name, kMargin, y,
                                 width - kMargin * 2, kRow * 2);
    y += kRow * 2 + kGap;

    const int colHeight = height - y - kMargin;
    MakeCard(page, kMargin - 6, y - 6, half + 12, colHeight + 12, kBgDark, kLine);

    g_statHead = MakeHeadingWithRule(page, kMargin, y, half, kHeadingHeight,
                                     TextOf("stat.head", "CHARACTER STATS"), kStatHeadName);
    y += kHeadingHeight + kGap;

    g_subject = MakeLabel(page, kSubjectName, kMargin, y, half, kRow);
    y += kRow + kTightGap;

    g_subjectRace = MakeLabel(page, kSubjectRaceName, kMargin, y, half, kRow);
    y += kRow + kTightGap;

    const int subjectButtonWidth = (half - kTightGap) / 2;
    BuildOutlineButton(page, kMargin, y, subjectButtonWidth, kButtonHeight,
                       kJumpRaceName, TextOf("subject.jumpRace", "Jump to race"),
                       kBrass, &g_jumpRaceBtn);
    g_jumpRace = g_jumpRaceBtn.button;

    BuildOutlineButton(page, kMargin + subjectButtonWidth + kTightGap, y,
                       half - subjectButtonWidth - kTightGap, kButtonHeight,
                       kEditAppearanceName, TextOf("subject.editAppearance", "Appearance"),
                       kBrass, &g_editAppearanceBtn);
    g_editAppearance = g_editAppearanceBtn.button;
    y += kButtonHeight + kGap * 2;

    g_statLabel = MakeLabel(page, kStatLabelName, kMargin, y, half, kLabelHeight);
    y += kLabelHeight + kTightGap;

    const int goWidth = 80;
    g_statSearch = MakeInput(page, kStatSearchName, kMargin, y,
                             half - goWidth - kTightGap, kRow);
    BuildOutlineButton(page, kMargin + half - goWidth, y, goWidth, kRow,
                       kStatSearchGoName, "Search", kBrass, &g_statSearchGoBtn);
    g_statSearchGo = g_statSearchGoBtn.button;
    y += kRow + kTightGap;

    g_statCombo = MakeCombo(page, kStatComboName, kMargin, y, half, kRow);
    y += kRow + kGap;

    g_statValueLabel = MakeLabel(page, kStatValueLabelName, kMargin, y, half, kLabelHeight);
    y += kLabelHeight + kTightGap;

    g_statValue = MakeInput(page, kStatValueName, kMargin, y, half, kRow);
    y += kRow + kTightGap;

    g_statBar = MakeScrollBar(page, kStatBarName, kMargin, y, half, kBarHeight);
    y += kBarHeight + kGap;

    BuildOutlineButton(page, kMargin, y, half, kButtonHeight,
                       kStatApplyName, TextOf("stat.apply", "Apply stat value"),
                       kAmber, &g_statApplyBtn);
    g_statApply = g_statApplyBtn.button;
    y += kButtonHeight + kGap;

    BuildPersonalityBlock(page, kMargin, y, half);

    // The right half: every skill this character has, with its value.
    const int rightLeft = kMargin * 2 + half;
    int rightY = kMargin + kRow * 2 + kGap;
    MakeCard(page, rightLeft - 6, rightY - 6, half + 12, colHeight + 12, kBgDark, kLine);

    g_skillListHead = MakeHeadingWithRule(page, rightLeft, rightY, half, kHeadingHeight,
                                          TextOf("skillList.head", "SKILL SHEET"), kSkillListHeadName);
    rightY += kHeadingHeight + kGap;

    int listHeight = height - rightY - kMargin;
    if (listHeight < kRow * 4)
    {
        listHeight = kRow * 4;
    }
    g_skillList = MakeList(page, kSkillListName, rightLeft, rightY,
                           half, listHeight);

    SetText(g_root, TextOf("window.title", "Total Editor"));

    // The warning is the most important text in the window. Two panels that
    // look alike do very different things: one edits every character of a race
    // in the world, the other edits one person.
    SetBodyText(g_warning, TextOf("window.warning",
        "RACE BONUSES ARE GLOBAL AND SHARED. Editing them changes EVERY "
        "character of that race in the world -- your squad, town guards and "
        "bandits alike. Bonuses are MULTIPLIERS (x1.15), not added points. "
        "The panel on the right edits one character's own skills (0-200)."));

    SetText(g_bonusDel, TextOf("bonus.remove", "[X]  Remove bonus"));
    SetTextColour(g_bonusDel, kDanger);
    SetText(g_bonusAdd, TextOf("bonus.add", "Add / update bonus"));
    SetTextColour(g_bonusAdd, kAmber);
    SetText(g_revert, TextOf("bonus.revert", "Revert race"));
    SetTextColour(g_revert, kDanger);
    SetText(g_statApply, TextOf("stats.apply", "Apply to character"));
    SetTextColour(g_statApply, kAmber);

    SetText(g_bonusHead, TextOf("heading.bonuses", "ONTO THIS RACE  (target)"));
    SetTextColour(g_bonusHead, kBrass);
    SetText(g_bonusListHead, TextOf("heading.currentBonuses", "WHAT THE TARGET CARRIES"));
    SetTextColour(g_bonusListHead, kBrass);
    SetText(g_statHead, TextOf("heading.stats", "CHARACTER SKILLS"));
    SetTextColour(g_statHead, kBrass);
    SetText(g_copyHead, TextOf("heading.copy", "COPY FROM THIS RACE  (source)"));
    SetTextColour(g_copyHead, kBrass);
    SetText(g_detailsHead, TextOf("heading.details", "WHAT THE SOURCE CARRIES"));
    SetTextColour(g_detailsHead, kBrass);
    SetText(g_copyButton, TextOf("copy.button", "Copy source onto target"));
    SetTextColour(g_copyButton, kAmber);
    SetText(g_filterGo, TextOf("filter.button", "Search"));
    SetTextColour(g_filterGo, kBrass);
    RefreshTraitToggle();
}

void FillDropdowns()
{
    const std::vector<RaceEntry>& races = Races(false);
    std::vector<std::string> raceNames;
    for (size_t i = 0; i < races.size(); ++i)
    {
        raceNames.push_back(races[i].name);
    }
    // Every dropdown is filled through its own filter, so they all start
    // showing everything and narrow independently as the player searches.
    RefreshRaceCombos();
    RefreshCopyCombo();
    RefreshStatCombo(g_bonusStat, g_bonusSearch, &g_visibleBonusStats);
    RefreshStatCombo(g_statCombo, g_statSearch, &g_visibleStats);

    // The two standalone tick boxes start matching the flags they control.
    SetTicked(g_realOnly, g_realRacesOnly);
    SetTicked(g_copyTraits, g_copyWithTraits);

    const std::vector<StatEntry>& stats = Stats();
    std::vector<std::string> statNames;
    for (size_t i = 0; i < stats.size(); ++i)
    {
        statNames.push_back(stats[i].name);
    }
    FillCombo(g_bonusStat, statNames, statNames.empty() ? -1 : 0);
    FillCombo(g_statCombo, statNames, statNames.empty() ? -1 : 0);
    FillCombo(g_personalityCombo, PersonalityLabels(), -1);

    const Config& config = GetConfig();
    SetScrollRange(g_bonusBar, config.bonusSteps);
    SetScrollPosition(g_bonusBar, BonusToStep(1.0f));
    SetInputText(g_bonusValue, FormatFloat(1.0f, 3));

    SetScrollRange(g_statBar, (int)(config.statMax - config.statMin));
    SetScrollPosition(g_statBar, 0);
}

bool Build()
{
    if (g_root != NULL)
    {
        return true;
    }

    const Config& config = GetConfig();
    int left = 0;
    int top = 0;
    ChoosePosition(config.windowWidth, config.windowHeight, &left, &top);

    g_root = MakeWindow(kWindowSkins, kWindowSkinCount, kRootName, kLayer,
                        left, top, config.windowWidth, config.windowHeight);
    if (g_root == NULL)
    {
        LogError("Could not create the editor window (no usable window skin).");
        return false;
    }

    g_client = ClientOf(g_root);
    if (g_client == NULL)
    {
        LogError("The editor window has no client area.");
        return false;
    }

    int clientWidth = config.windowWidth;
    int clientHeight = config.windowHeight;
    ClientSize(g_client, &clientWidth, &clientHeight);

    // Lacquer background fill covering the entire client area
    MakeFill(g_client, 0, 0, clientWidth, clientHeight, kLacquer, "TE_LacquerBg");
    // 1px gold frame around the client area
    MakeFrame(g_client, 0, 0, clientWidth, clientHeight, 1, kGoldDim);

    // The warning spans the full width and sits above the tabs, because it is
    // true on every page: it is what stops someone editing every bandit in the
    // world by accident.
    // At 1280px window width, the warning comfortably fits in 2 lines (44px)
    // with no vertical scrollbar, removing empty space and giving ~90px more height to page content.
    const int warningHeight = 44;
    g_warning = MakeStaticText(g_client, kWarningName, kMargin, kMargin,
                               clientWidth - kMargin * 2, warningHeight);
    SetTextColour(g_warning, kAmber);

    const int statusTop = clientHeight - kMargin - kRow;
    g_status = MakeLabel(g_client, kStatusName, kMargin, statusTop,
                         clientWidth - kMargin * 2, kRow);
    SetTextColour(g_status, kMuted);

    SetTextColour(g_root, kBrass);
    SetTextShadow(g_root, true);

    // A row of buttons and a stack of panels, rather than MyGUI::TabControl:
    // that control links, accepts its skin, and draws nothing at all -- no tab
    // strip and no page. This is plainer and it is visible.
    const int stripTop = kMargin + warningHeight + kGap;
    const int tabWidth = (clientWidth - kMargin * 2) / kPageCount;

    const char* const tabKeys[kPageCount] =
        { "tab.overview", "tab.bonuses", "tab.parts", "tab.flags", "tab.character", "tab.uniform" };
    const char* const tabNames[kPageCount] =
        { "Race overview", "Skill bonuses", "Body parts",
          "Equipment and nature", "Character", "Uniform & Portrait" };

    // The pages are created FIRST, so the tab strip ends up above them in
    // z-order rather than underneath. On its own that is belt and braces -- the
    // panels no longer take the mouse either -- but a strip the pages can cover
    // is the bug that froze this window on page one.
    const int pageTop = stripTop + kButtonHeight + kGap;
    const int pageWidth = clientWidth - kMargin * 2;
    const int pageHeight = statusTop - pageTop - kGap;

    for (int i = 0; i < kPageCount; ++i)
    {
        char name[32] = { 0 };
        _snprintf_s(name, sizeof(name), _TRUNCATE, "TE_Page%d", i);
        g_pages[i] = MakePanel(g_client, name, kMargin, pageTop,
                               pageWidth, pageHeight);
        if (g_pages[i] == NULL)
        {
            LogError("Could not create a page panel.");
            return false;
        }
    }

    for (int i = 0; i < kPageCount; ++i)
    {
        char name[32] = { 0 };
        _snprintf_s(name, sizeof(name), _TRUNCATE, "TE_Tab%d", i);
        BuildOutlineButton(g_client, kMargin + i * tabWidth, stripTop,
                           tabWidth - 4, kButtonHeight, name,
                           TextOf(tabKeys[i], tabNames[i]),
                           (i == 0) ? kAmber : kLine, &g_tabs[i],
                           (i == 0) ? kSumi : kBgDark);
        g_tabButtons[i] = g_tabs[i].button;
        if (i != 0)
        {
            SetOutlineButtonColour(&g_tabs[i], kLine, kMuted, kBgDark);
        }
    }

    BuildOverviewPage(g_pages[0], pageWidth, pageHeight);
    BuildBonusPage(g_pages[1], pageWidth, pageHeight);
    BuildPartPage(g_pages[2], pageWidth, pageHeight);
    BuildFlagPage(g_pages[3], pageWidth, pageHeight);
    BuildCharacterPage(g_pages[4], pageWidth, pageHeight);
    BuildUniformPage(g_pages[5], pageWidth, pageHeight);

    ApplyLabels();
    FillDropdowns();
    BindEvents();

    {
        int pages = 0;
        int buttons = 0;
        for (int i = 0; i < kPageCount; ++i)
        {
            if (g_pages[i] != NULL) { ++pages; }
            if (g_tabButtons[i] != NULL) { ++buttons; }
        }

        char detail[96] = { 0 };
        _snprintf_s(detail, sizeof(detail), _TRUNCATE,
                    "pages %d, tab buttons %d, client %dx%d",
                    pages, buttons, clientWidth, clientHeight);
        LogInfo("Window built: " + std::string(detail) + ".");
    }

    ShowPage(0);
    SetVisible(g_root, false);
    return true;
}
} // namespace

bool IsEditorOpen()
{
    return g_open;
}

void ApplyStoredOverridesIfNeeded()
{
    if (g_overridesApplied || !GetConfig().applyOnStart)
    {
        return;
    }

    // Refresh the race catalogue so we have fresh pointers for the loaded session
    Races(true);

    const int applied = ApplyStoredOverrides();
    g_overridesApplied = true;

    char count[16] = { 0 };
    _snprintf_s(count, sizeof(count), _TRUNCATE, "%d", applied);
    LogInfo("Stored overrides applied to " + std::string(count) + " races.");
}

namespace
{
void CheckWorldInitHelper()
{
    TE_TRY
    {
        if (ou != NULL)
        {
            if (ou->gameResetting)
            {
                // Game is being unloaded/reset (e.g. returning to menu or reloading save).
                g_overridesApplied = false;
            }
            else if (ou->initialized && !g_overridesApplied)
            {
                ApplyStoredOverridesIfNeeded();
            }
        }
    }
    TE_CATCH { }
}
} // namespace

void EditorTick()
{
    // Auto-apply overrides as soon as a game world is active, even if the editor window is closed.
    CheckWorldInitHelper();

    // Both run every frame, window open or not: the engine drains the save
    // hooks and the portrait editor drains its composition queue.
    UniformEngine::Tick();
    PortraitEditor::Tick();

    if (!g_open)
    {
        return;
    }

    // The uniform page follows the selection regardless of followSelection:
    // it holds a live preview on a specific character.
    UcFollow(SelectedCharacter());
    if (g_currentPage == 5 && g_ucChar != NULL && g_ucPortrait != NULL
        && (++g_ucFrame % 30) == 0)
    {
        PortraitEditor::Decorate(g_ucChar, g_ucPortrait);
    }

    if (!GetConfig().followSelection)
    {
        return;
    }

    Character* selected = SelectedCharacter();
    if (selected == g_shownCharacter)
    {
        return;
    }

    g_shownCharacter = selected;
    RefreshSubject();
    RefreshSkillList();
    RefreshPersonality();

    // The race panels follow too, by pointing the picker at the selected
    // character's own race. Body parts and blood are race data, so this is what
    // makes them describe the person on screen rather than whichever race was
    // left in the combo -- the same move the "Edit this race" button makes,
    // without having to press it.
    //
    // The source picker on page one is deliberately left alone: it is the race
    // being copied FROM, which is a choice the player made and not a property
    // of who they just clicked.
    const std::string raceName = RaceNameOf(RaceOf(selected));
    if (!raceName.empty() && raceName != CurrentRaceName())
    {
        SelectRaceByName(raceName);
    }
}

void HideEditor()
{
    if (g_open)
    {
        Close();
    }
}

void ToggleEditor()
{
    if (g_open)
    {
        Close();
        return;
    }

    if (!Build())
    {
        return;
    }

    g_open = true;

    // Races are re-read on open rather than cached forever: loading a save can
    // bring in mods that were not there when the plugin started.
    Races(true);

    ApplyStoredOverridesIfNeeded();

    const std::string raceName = CurrentRaceName();
    RaceData* race = CurrentRace();
    if (race != NULL && !raceName.empty())
    {
        RememberOriginal(raceName, race);
    }

    RefreshBonusList();
    RefreshSubject();
    RefreshSkillList();
    RefreshPersonality();

    // The details panel describes the SOURCE race, which is the left-hand
    // combo. RefreshBonusList follows the target on the right, so on open the
    // panel had never been told what the source picker is sitting on and
    // showed whatever it was left holding.
    RefreshRaceDetails();

    SetStatus(TextOf("status.ready", "Ready."), kReady);
    SetVisible(g_root, true);
}

void ShutdownEditor()
{
    // Pending edits are flushed before the widgets go: losing them silently on
    // shutdown is exactly the bug a player would report as "it forgot".
    Flush();

    g_open = false;
    Unbind();

    // Children go with the root, so only the root is destroyed.
    Destroy(g_root);

    g_root = NULL;
    g_client = NULL;
    g_warning = NULL;
    g_raceCombo = NULL;
    g_bonusStat = NULL;
    g_bonusValue = NULL;
    g_bonusBar = NULL;
    g_bonusAdd = NULL;
    g_bonusDel = NULL;
    g_revert = NULL;
    g_subject = NULL;
    g_statCombo = NULL;
    g_statValue = NULL;
    g_statBar = NULL;
    g_statApply = NULL;
    g_status = NULL;
    g_copyCombo = NULL;
    g_copyButton = NULL;
    g_bonusHead = NULL;
    g_statHead = NULL;
    g_copyHead = NULL;
    g_filter = NULL;
    g_filterGo = NULL;
    g_copyTraits = NULL;
    g_subjectRace = NULL;
    g_jumpRace = NULL;
    g_editAppearance = NULL;
    g_editHead = NULL;
    g_raceLabel = NULL;
    g_bonusLabel = NULL;
    g_bonusSearch = NULL;
    g_bonusSearchGo = NULL;
    g_bonusValueLabel = NULL;
    g_copyLabel = NULL;
    g_copySearch = NULL;
    g_copySearchGo = NULL;
    g_statLabel = NULL;
    g_statSearch = NULL;
    g_statSearchGo = NULL;
    g_statValueLabel = NULL;
    g_details = NULL;
    g_detailsHead = NULL;
    g_partList = NULL;
    g_partValue = NULL;
    g_partApply = NULL;
    g_partHead = NULL;
    g_partLabel = NULL;
    g_realOnly = NULL;
    g_hitValue = NULL;
    g_hitLabel = NULL;
    g_hitApply = NULL;
    g_flagList = NULL;
    g_flagToggle = NULL;
    g_flagHead = NULL;
    for (int i = 0; i < kPageCount; ++i)
    {
        g_pages[i] = NULL;
        g_tabButtons[i] = NULL;
        g_tabDelegates[i] = NULL;
        g_tabs[i] = OutlineButton();
    }
    g_realOnlyLabel = NULL;
    g_copyTraitsLabel = NULL;
    g_pageHelp1 = NULL;
    g_pageHelp2 = NULL;
    g_pageHelp3 = NULL;
    g_pageHelp4 = NULL;
    g_pageHelp5 = NULL;
    g_bonusListHead = NULL;
    g_bloodHead = NULL;
    g_bloodMinLabel = NULL;
    g_bloodMin = NULL;
    g_bloodMaxLabel = NULL;
    g_bloodMax = NULL;
    g_bloodApply = NULL;
    g_bloodApplyDelegate = NULL;
    g_skillList = NULL;
    g_skillListHead = NULL;
    g_skillListDelegate = NULL;
    g_raceBonusList = NULL;
    g_raceBonusHead = NULL;
    g_raceBonusDelegate = NULL;
    g_shownRaceBonuses.clear();
    g_targetDetails = NULL;
    g_personalityHead = NULL;
    g_personalityCurrent = NULL;
    g_personalityCombo = NULL;
    g_personalityApply = NULL;
    g_personalityApplyDelegate = NULL;

    for (int i = 0; i < kFlagRowCount; ++i)
    {
        g_flagBoxes[i] = NULL;
        g_flagLabels[i] = NULL;
        g_flagBoxDelegates[i] = NULL;
    }

    g_closeDelegate = NULL;
    g_bonusAddDelegate = NULL;
    g_bonusDelDelegate = NULL;
    g_revertDelegate = NULL;
    g_statApplyDelegate = NULL;
    g_raceDelegate = NULL;
    g_statComboDelegate = NULL;
    g_copyComboDelegate = NULL;
    g_bonusBarDelegate = NULL;
    g_statBarDelegate = NULL;
    g_copyDelegate = NULL;
    g_filterDelegate = NULL;
    g_copyTraitsDelegate = NULL;
    g_jumpRaceDelegate = NULL;
    g_editAppearanceDelegate = NULL;
    g_bonusSearchDelegate = NULL;
    g_copySearchDelegate = NULL;
    g_statSearchDelegate = NULL;
    g_partApplyDelegate = NULL;
    g_partListDelegate = NULL;
    g_realOnlyDelegate = NULL;
    g_hitApplyDelegate = NULL;
    g_flagToggleDelegate = NULL;
    g_flagListDelegate = NULL;
    g_shownParts.clear();
    g_shownCharacter = NULL;
    g_visibleBonusStats.clear();
    g_visibleStats.clear();
    g_visibleCopyRaces.clear();
    g_visibleRaces.clear();
    g_overridesApplied = false;

    g_pageHelp6 = NULL;
    g_ucPortrait = NULL;
    g_ucName = NULL;
    g_ucEditPortrait = NULL;
    g_ucVanilla = NULL;
    g_ucSave = NULL;
    g_ucCancel = NULL;
    g_ucChar = NULL;
    g_ucVanillaDelegate = NULL;
    g_ucSaveDelegate = NULL;
    g_ucCancelDelegate = NULL;
    g_ucEditPortraitDelegate = NULL;
    g_ucPrimaryHexDelegate = NULL;
    g_ucSecondaryHexDelegate = NULL;
    for (int b = 0; b < 2; ++b)
    {
        for (int i = 0; i < 8; ++i)
        {
            g_ucStepDelegates[b][i] = NULL;
        }
    }
}
} // namespace totaleditor
