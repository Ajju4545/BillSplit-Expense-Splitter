/*
 ██████╗ ██╗██╗     ██╗      ███████╗██████╗ ██╗     ██╗████████╗
 ██╔══██╗██║██║     ██║      ██╔════╝██╔══██╗██║     ██║╚══██╔══╝
 ██████╔╝██║██║     ██║      ███████╗██████╔╝██║     ██║   ██║
 ██╔══██╗██║██║     ██║      ╚════██║██╔═══╝ ██║     ██║   ██║
 ██████╔╝██║███████╗███████╗ ███████║██║     ███████╗██║   ██║
 ╚═════╝ ╚═╝╚══════╝╚══════╝ ╚══════╝╚═╝     ╚══════╝╚═╝   ╚═╝
          E X P E N S E   B I L L   S P L I T T E R
                    [ MINI PROJECT ]
*/

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <ctime>
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#ifdef _WIN32
  #include <windows.h>
#endif
using namespace std;

// ════════════════════════════════════════════
//  ANSI COLOUR CODES
// ════════════════════════════════════════════
#define RST   "\033[0m"
#define BOLD  "\033[1m"
#define DIM   "\033[2m"
#define RED   "\033[91m"
#define GRN   "\033[92m"
#define YLW   "\033[93m"
#define BLU   "\033[94m"
#define MAG   "\033[95m"
#define CYN   "\033[96m"
#define WHT   "\033[97m"
#define GRY   "\033[90m"
#define BG_RED  "\033[41m"
#define BG_GRN  "\033[42m"
#define BG_YLW  "\033[43m"

const int W = 58;

// ════════════════════════════════════════════
//  DATA STRUCTURES
// ════════════════════════════════════════════

// One expense entry inside a trip
struct Expense {
    string category;
    string note;
    vector<double> consumed;   // consumed[i] = how much person i spent/ate (their share)
    vector<double> paidFor;    // paidFor[i]  = how much person i actually paid upfront
    vector<bool> participating;
    double total;              // sum of consumed
};

// One trip contains many expenses
struct Trip {
    string name;
    vector<Expense> expenses;
    double total;             // sum of all expenses in this trip
};

// ════════════════════════════════════════════
//  UTILITY HELPERS
// ════════════════════════════════════════════
string centre(const string& s, int w = W) {
    int pad = (w - (int)s.size()) / 2;
    if (pad < 0) pad = 0;
    return string(pad, ' ') + s;
}
string rep(char c, int n) { return string(n, c); }

void sep(char c = '-', const string& col = GRY) {
    cout << col << rep(c, W) << RST << "\n";
}
void cprint(const string& s, const string& col = WHT) {
    cout << col << BOLD << centre(s) << RST << "\n";
}
string timestamp() {
    time_t now = time(nullptr);
    char buf[32];
    strftime(buf, sizeof(buf), "%d-%m-%Y  %H:%M:%S", localtime(&now));
    return string(buf);
}
string rupee(double v) {
    ostringstream ss;
    ss << fixed << setprecision(2) << v;
    return "Rs." + ss.str();
}
void progressBar(const string& label, const string& col = CYN) {
    cout << col << "  " << label << "  [";
    for (int i = 0; i < 20; i++) {
        cout << "\xe2\x96\x88" << flush;
#ifdef _WIN32
        Sleep(25);
#endif
    }
    cout << "] DONE" << RST << "\n";
}

// ════════════════════════════════════════════
//  CATEGORY SELECTOR
// ════════════════════════════════════════════
string selectCategory(int n_people) {
    (void)n_people;
    cout << "\n" << CYN << BOLD;
    sep('-', CYN);
    cprint("  SELECT CATEGORY  ", CYN);
    sep('-', CYN);
    cout << RST;
    cout << GRN  << "  [1]" << WHT << "  Lunch / Food\n"     << RST;
    cout << YLW  << "  [2]" << WHT << "  Shopping\n"          << RST;
    cout << BLU  << "  [3]" << WHT << "  Travel / Fuel\n"     << RST;
    cout << MAG  << "  [4]" << WHT << "  Hotel / Stay\n"      << RST;
    cout << CYN  << "  [5]" << WHT << "  Entertainment\n"     << RST;
    cout << RED  << "  [6]" << WHT << "  Medical\n"           << RST;
    cout << GRY  << "  [7]" << WHT << "  Other (Custom)\n"    << RST;
    sep('-', CYN);
    cout << WHT << "  » Enter choice (1-7): " << YLW;

    int cat; cin >> cat; cout << RST;
    string category;
    switch (cat) {
        case 1: category = "Lunch / Food";    break;
        case 2: category = "Shopping";        break;
        case 3: category = "Travel / Fuel";   break;
        case 4: category = "Hotel / Stay";    break;
        case 5: category = "Entertainment";   break;
        case 6: category = "Medical";         break;
        case 7: {
            cin.ignore();
            cout << WHT << "  Enter custom category name: " << YLW;
            getline(cin, category);
            if (category.empty()) category = "Other";
            cout << RST;
            break;
        }
        default:
            category = "Other";
            cout << GRY << "  (Invalid, set to Other)\n" << RST;
    }
    cout << GRN << "  \xe2\x9c\x94 Category: " << BOLD << category << RST << "\n";
    return category;
}

// ════════════════════════════════════════════
//  ADD EXPENSE (one category item)
// ════════════════════════════════════════════
Expense addExpense(const vector<string>& names, int n) {
    Expense exp;
    exp.consumed.resize(n, 0.0);
    exp.paidFor.resize(n, 0.0);
    exp.participating.resize(n, false);
    exp.total = 0;

    // Category
    exp.category = selectCategory(n);
    exp.note = exp.category;

    // Step 1: How much did each person consume/spend? (0 = not participating)
    cout << "\n" << CYN << BOLD << "  STEP 1: Amount consumed by each person\n" << RST;
    cout << GRY  << "  (Enter 0 if person did not participate)\n" << RST;
    for (int i = 0; i < n; i++) {
        double amt;
        do {
            cout << "  " << left << setw(14)
                 << (string("[") + names[i] + "]")
                 << " consumed Rs. " << YLW;
            cin >> amt;
            cout << RST;
            if (amt < 0) cout << RED << "  \xe2\x9c\x96 Cannot be negative!\n" << RST;
        } while (amt < 0);
        exp.consumed[i] = amt;
        exp.total += amt;
        if (amt > 0) exp.participating[i] = true;
    }
    cout << GRN << "  Total expense: " << BOLD << "Rs." << fixed << setprecision(2) << exp.total << RST << "\n";

    // Step 2: Who actually paid the bill upfront?
    cout << "\n" << CYN << BOLD << "  STEP 2: Who paid the bill upfront?\n" << RST;
    cout << GRY  << "  (Enter 0 if this person did not pay. Total must equal Rs."
         << fixed << setprecision(2) << exp.total << ")\n" << RST;
    double totalPaidUpfront = 0;
    for (int i = 0; i < n; i++) {
        double amt;
        do {
            cout << "  " << left << setw(14)
                 << (string("[") + names[i] + "]")
                 << " paid Rs. " << YLW;
            cin >> amt;
            cout << RST;
            if (amt < 0) cout << RED << "  \xe2\x9c\x96 Cannot be negative!\n" << RST;
        } while (amt < 0);
        exp.paidFor[i] = amt;
        totalPaidUpfront += amt;
    }
    // Warn if mismatch
    if (fabs(totalPaidUpfront - exp.total) > 0.01) {
        cout << YLW << "  Warning: Amount paid (Rs." << fixed << setprecision(2) << totalPaidUpfront
             << ") != total consumed (Rs." << exp.total << "). Adjusting...\n" << RST;
        // Scale paidFor to match total
        if (totalPaidUpfront > 0)
            for (int i = 0; i < n; i++)
                exp.paidFor[i] = exp.paidFor[i] / totalPaidUpfront * exp.total;
    }
    return exp;
}

// ════════════════════════════════════════════
//  SPLASH SCREEN
// ════════════════════════════════════════════
void splash() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode;
    GetConsoleMode(h, &mode);
    SetConsoleMode(h, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#endif
    cout << "\n\n";
    cout << MAG << BOLD;
    cout << centre("╔════════════════════════════════════════════════════════╗") << "\n";
    cout << centre("║                                                        ║") << "\n";
    cout << centre("║   ██████╗ ██╗██╗     ██╗      ███████╗██████╗          ║") << "\n";
    cout << centre("║   ██╔══██╗██║██║     ██║      ██╔════╝██╔══██╗         ║") << "\n";
    cout << centre("║   ██████╔╝██║██║     ██║      ███████╗██████╔╝         ║") << "\n";
    cout << centre("║   ██╔══██╗██║██║     ██║      ╚════██║██╔═══╝          ║") << "\n";
    cout << centre("║   ██████╔╝██║███████╗███████╗ ███████║██║              ║") << "\n";
    cout << centre("║   ╚═════╝ ╚═╝╚══════╝╚══════╝ ╚══════╝╚═╝              ║") << "\n";
    cout << centre("║                                                        ║") << "\n";
    cout << CYN  << centre("║        EXPENSE  BILL  SPLITTER  v3.0                   ║") << "\n";
    cout << YLW  << centre("║          [ Multi-Category  Edition ]                   ║") << "\n";
    cout << MAG  << centre("╚════════════════════════════════════════════════════════╝") << "\n";
    cout << RST << "\n";
    cout << GRY << centre("Initialising system...") << RST << "\n\n";
    progressBar("Loading modules   ", GRN);
    progressBar("Setting up ledger ", CYN);
    progressBar("Ready             ", YLW);
    cout << "\n" << GRN << BOLD << centre("✔  System Ready!") << RST << "\n\n";
}

// ════════════════════════════════════════════
//  RECEIPT
// ════════════════════════════════════════════
void printReceipt(
    const string& groupName,
    const vector<string>& names,
    const vector<double>& totalPaid,
    const vector<Trip>& trips,
    double grandTotal, int n)
{
    // fairShare[i] = total consumed by person i across all expenses
    // totalPaid[i] = total paid upfront by person i
    // balance = totalPaid - fairShare: positive means gets back, negative means owes
    vector<double> fairShare(n, 0.0);
    for (auto& tr : trips)
        for (auto& ex : tr.expenses)
            for (int i = 0; i < n; i++)
                fairShare[i] += ex.consumed[i];

    cout << "\n" << YLW << centre(rep('~', W)) << RST << "\n";
    cout << YLW << BOLD << centre("  *** PRINTING RECEIPT ***  ") << RST << "\n";
    cout << YLW << centre(rep('~', W)) << RST << "\n\n";

    // Header
    sep('=', MAG);
    cprint("BILLSPLIT  RECEIPT", MAG);
    cprint("Powered by BillSplit v3.0", GRY);
    sep('-', GRY);
    cout << GRY << "  Group  : " << WHT << BOLD << groupName << RST << "\n";
    cout << GRY << "  Date   : " << WHT << timestamp() << RST << "\n";
    sep('=', MAG);

    // Per-trip expense breakdown
    for (int t = 0; t < (int)trips.size(); t++) {
        const Trip& trip = trips[t];
        cout << "\n" << YLW << BOLD
             << "  TRIP #" << t+1 << "  " << trip.name
             << "  (" << rupee(trip.total) << ")\n" << RST;
        sep('-', GRY);

        for (int e = 0; e < (int)trip.expenses.size(); e++) {
            const Expense& ex = trip.expenses[e];
            cout << CYN << "  [" << ex.category << "]"
                 << GRY << "  " << ex.note
                 << WHT << "  => " << rupee(ex.total) << RST << "\n";
            for (int i = 0; i < n; i++) {
                if (ex.consumed[i] > 0 || ex.paidFor[i] > 0) {
                    cout << GRY << "     |- " << left << setw(13) << names[i];
                    if (ex.consumed[i] > 0)
                        cout << WHT << "consumed " << rupee(ex.consumed[i]);
                    if (ex.paidFor[i] > 0)
                        cout << GRN << "  paid " << rupee(ex.paidFor[i]);
                    cout << RST << "\n";
                }
            }
        }
        sep('-', GRY);
        cout << GRY << "  Trip Subtotal : " << YLW << BOLD << rupee(trip.total) << RST << "\n";
    }

    // Grand totals
    cout << "\n";
    sep('=', GRY);
    cout << GRY  << "  Grand Total      " << WHT << BOLD << right << setw(20) << rupee(grandTotal) << RST << "\n";
    cout << GRY  << "  People           " << WHT << BOLD << right << setw(20) << n               << RST << "\n";
    sep('-', GRY);

    // Balance sheet
    cout << "\n" << BOLD << WHT << "  BALANCE SHEET :\n" << RST << "\n";
    for (int i = 0; i < n; i++) {
        double bal = totalPaid[i] - fairShare[i];
        string avatar = "  [" + string(1, toupper(names[i][0])) + "] ";
        cout << (bal >= 0 ? GRN : RED) << BOLD << avatar << RST;
        cout << WHT << BOLD << left << setw(12) << names[i] << RST;
        cout << DIM << GRY << "paid " << rupee(totalPaid[i]) << "  share " << rupee(fairShare[i]) << RST;
        if (bal > 0.009)
            cout << "  " << BG_GRN << BOLD << " ^ GETS " << rupee(bal)  << " " << RST << "\n";
        else if (bal < -0.009)
            cout << "  " << BG_RED << BOLD << " v OWES " << rupee(-bal) << " " << RST << "\n";
        else
            cout << "  " << BG_YLW << BOLD << " = SETTLED         " << RST << "\n";

        int barLen = (int)(totalPaid[i] / grandTotal * 30);
        cout << "       " << GRY;
        for (int b = 0; b < 30; b++) cout << (b < barLen ? "\xe2\x96\x88" : "\xe2\x96\x91");
        cout << RST << "\n\n";
    }
    sep('-', GRY);

    // Settlement plan
    cout << "\n" << MAG << BOLD << "  SETTLEMENT PLAN :\n" << RST << "\n";
    vector<pair<double,int>> balVec(n);
    for (int i = 0; i < n; i++) balVec[i] = { totalPaid[i] - fairShare[i], i };
    bool anyTx = false; int txNum = 1;
    for (int iter = 0; iter < n*n; iter++) {
        int cred=-1, debt=-1;
        double maxC=1e-9, maxD=1e-9;
        for (int i=0;i<n;i++){
            if (balVec[i].first >  maxC){ maxC =  balVec[i].first; cred=i; }
            if (balVec[i].first < -maxD){ maxD = -balVec[i].first; debt=i; }
        }
        if (cred==-1||debt==-1) break;
        double amt = min(maxC,maxD);
        cout << "  " << YLW << BOLD << "[TX" << txNum++ << "] " << RST;
        cout << RED  << BOLD << left << setw(10) << names[balVec[debt].second] << RST;
        cout << WHT  << "  -->  ";
        cout << GRN  << BOLD << left << setw(10) << names[balVec[cred].second] << RST;
        cout << CYN  << BOLD << "  " << rupee(amt) << RST << "\n";
        balVec[cred].first -= amt;
        balVec[debt].first += amt;
        anyTx = true;
    }
    if (!anyTx) cout << GRN << BOLD << centre("Everyone is SETTLED!") << RST << "\n";

    // Category summary
    cout << "\n";
    sep('=', CYN);
    cprint("  CATEGORY-WISE SUMMARY  ", CYN);
    sep('-', GRY);
    map<string,double> catTotals;
    for (auto& tr : trips)
        for (auto& ex : tr.expenses)
            catTotals[ex.category] += ex.total;
    for (auto& kv : catTotals) {
        int bars = (int)(kv.second / grandTotal * 28);
        cout << "  " << left << setw(16) << kv.first
             << YLW;
        for (int b=0;b<28;b++) cout << (b<bars ? "\xe2\x96\x93" : "\xe2\x96\x91");
        cout << RST << "  " << WHT << rupee(kv.second) << RST << "\n";
    }
    sep('=', CYN);

    // Footer
    cout << "\n";
    sep('=', MAG);
    cprint("Thank you for using BillSplit!", MAG);
    cprint("Split smart. Travel happy.", GRY);
    sep('=', MAG);
    cout << "\n";
}

// ════════════════════════════════════════════
//  MENU
// ════════════════════════════════════════════
void showMenu(const string& groupName, int tripCount, double total) {
    cout << "\n";
    sep('-', GRY);
    cout << CYN << BOLD << "  MENU" << RST;
    cout << GRY << "  [Group: " << WHT << groupName
         << GRY << "  |  Trips: " << WHT << tripCount
         << GRY << "  |  Total: " << YLW << rupee(total) << GRY << "]\n" << RST;
    sep('-', GRY);
    cout << GRN << "  [1]" << WHT << "  Add New Trip\n"          << RST;
    cout << BLU << "  [2]" << WHT << "  Add Expense to Trip\n"   << RST;
    cout << CYN << "  [3]" << WHT << "  View Trip History\n"     << RST;
    cout << MAG << "  [4]" << WHT << "  Print Final Receipt\n"   << RST;
    cout << YLW << "  [5]" << WHT << "  Group Stats\n"           << RST;
    cout << RED << "  [6]" << WHT << "  Reset All Data\n"        << RST;
    cout << GRY << "  [7]" << WHT << "  Exit\n"                  << RST;
    sep('-', GRY);
    cout << WHT << "  >> Enter choice: " << YLW;
}

// ════════════════════════════════════════════
//  GROUP STATS
// ════════════════════════════════════════════
void showStats(
    const vector<string>& names,
    const vector<double>& totalPaid,
    const vector<Trip>& trips,
    double grandTotal, int n,
    const string& groupName)
{
    cout << "\n";
    sep('=', CYN);
    cprint("GROUP STATS", CYN);
    sep('-', GRY);
    cout << GRY << "  Group      : " << WHT << groupName << "\n";
    cout << GRY << "  Members    : " << WHT << n << "\n";
    cout << GRY << "  Trips      : " << WHT << trips.size() << "\n";
    int totalExp = 0;
    for (auto& tr : trips) totalExp += tr.expenses.size();
    cout << GRY << "  Expenses   : " << WHT << totalExp << "\n";
    cout << GRY << "  Grand Total: " << YLW << rupee(grandTotal) << "\n\n" << RST;

    int maxIdx = max_element(totalPaid.begin(), totalPaid.end()) - totalPaid.begin();
    int minIdx = min_element(totalPaid.begin(), totalPaid.end()) - totalPaid.begin();
    cout << GRN << BOLD << "  Top Payer  : " << names[maxIdx] << "  " << rupee(totalPaid[maxIdx]) << RST << "\n";
    cout << RED << BOLD << "  Low Payer  : " << names[minIdx] << "  " << rupee(totalPaid[minIdx]) << RST << "\n\n";

    // Contribution bars
    cout << CYN << "  CONTRIBUTION:\n" << RST;
    sep('-', GRY);
    for (int i=0;i<n;i++){
        double pct = (grandTotal>0)?(totalPaid[i]/grandTotal*100.0):0;
        int bars = (int)(pct/100.0*30);
        cout << "  " << left << setw(12) << names[i] << YLW;
        for (int b=0;b<30;b++) cout << (b<bars?"\xe2\x96\x93":"\xe2\x96\x91");
        cout << RST << "  " << GRY << fixed << setprecision(1) << pct << "%" << RST << "\n";
    }

    // Per-category breakdown
    cout << "\n" << CYN << "  CATEGORY BREAKDOWN:\n" << RST;
    sep('-', GRY);
    map<string,double> catTotals;
    for (auto& tr : trips)
        for (auto& ex : tr.expenses)
            catTotals[ex.category] += ex.total;
    for (auto& kv : catTotals) {
        double pct = kv.second / grandTotal * 100.0;
        cout << "  " << left << setw(18) << kv.first
             << WHT << right << setw(10) << rupee(kv.second)
             << GRY << "  (" << fixed << setprecision(1) << pct << "%)" << RST << "\n";
    }
    sep('=', CYN);
}

// ════════════════════════════════════════════
//  MAIN
// ════════════════════════════════════════════
int main() {
    splash();

    // Group name
    cout << MAG << BOLD << "  +-- GROUP SETUP ----------------------------------+\n" << RST;
    cout << WHT << "  Enter group/trip name: " << YLW;
    string groupName;
    getline(cin, groupName);
    if (groupName.empty()) groupName = "My Group";
    cout << RST;

    // Number of people
    int n;
    do {
        cout << WHT << "  Number of people (2-10): " << YLW;
        cin >> n; cout << RST;
        if (n < 2 || n > 10)
            cout << RED << "  Enter between 2 and 10!\n" << RST;
    } while (n < 2 || n > 10);

    // Names
    vector<string> names(n);
    vector<double> totalPaid(n, 0.0);
    cout << "\n" << CYN << "  Enter member names:\n" << RST;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    for (int i = 0; i < n; i++) {
        cout << GRY << "  Member " << i+1 << " -> " << YLW;
        getline(cin, names[i]);
        if (names[i].empty()) names[i] = "Person" + to_string(i+1);
        cout << RST;
    }
    cout << GRN << "\n  Group \"" << groupName << "\" created with " << n << " members!\n" << RST;

    vector<Trip> trips;
    double grandTotal = 0;
    int choice;

    do {
        showMenu(groupName, (int)trips.size(), grandTotal);
        cin >> choice;
        cout << RST;

        switch(choice) {

        // ── ADD NEW TRIP ──────────────────────────
        case 1: {
            cout << "\n" << GRN << BOLD;
            sep('-', GRN);
            cprint("  ADD NEW TRIP  ", GRN);
            sep('-', GRN);
            cout << RST;

            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << WHT << "  Trip name: " << YLW;
            string tname;
            getline(cin, tname);
            if (tname.empty()) tname = "Trip #" + to_string(trips.size()+1);
            cout << RST;

            Trip trip;
            trip.name  = tname;
            trip.total = 0;

            // Add first expense immediately
            cout << GRN << "\n  Add first expense to this trip:\n" << RST;
            Expense exp = addExpense(names, n);
            trip.expenses.push_back(exp);
            trip.total += exp.total;
            for (int i=0;i<n;i++){
                totalPaid[i] += exp.paidFor[i];
                grandTotal   += exp.paidFor[i];
            }

            // Ask if more expenses to add
            char more;
            do {
                cout << "\n" << YLW << BOLD
                     << "  Add another expense to \"" << tname << "\"? (y/n): " << RST << YLW;
                cin >> more; cout << RST;
                if (more == 'y' || more == 'Y') {
                    Expense exp2 = addExpense(names, n);
                    trip.expenses.push_back(exp2);
                    trip.total += exp2.total;
                    for (int i=0;i<n;i++){
                        totalPaid[i] += exp2.paidFor[i];
                        grandTotal   += exp2.paidFor[i];
                    }
                    cout << GRN << "  Expense added! Trip total so far: "
                         << BOLD << rupee(trip.total) << RST << "\n";
                }
            } while (more == 'y' || more == 'Y');

            trips.push_back(trip);
            progressBar("Saving trip       ", GRN);
            cout << GRN << BOLD << "  Trip \"" << tname << "\" saved!"
                 << "  Expenses: " << trip.expenses.size()
                 << "  |  Total: " << rupee(trip.total) << RST << "\n";
            break;
        }

        // ── ADD EXPENSE TO EXISTING TRIP ─────────
        case 2: {
            if (trips.empty()) {
                cout << RED << "  No trips yet! Add a trip first (option 1).\n" << RST;
                break;
            }
            // Show trip list
            cout << "\n" << YLW << BOLD;
            sep('-', YLW);
            cprint("  SELECT TRIP  ", YLW);
            sep('-', YLW); cout << RST;
            for (int t=0;t<(int)trips.size();t++)
                cout << YLW << "  [" << t+1 << "]" << WHT << "  " << trips[t].name
                     << GRY << "  (" << trips[t].expenses.size() << " expenses, "
                     << rupee(trips[t].total) << ")\n" << RST;
            sep('-', GRY);
            cout << WHT << "  >> Select trip (1-" << trips.size() << "): " << YLW;
            int tidx; cin >> tidx; cout << RST;
            if (tidx < 1 || tidx > (int)trips.size()) {
                cout << RED << "  Invalid choice!\n" << RST; break;
            }
            tidx--;

            Expense exp = addExpense(names, n);
            trips[tidx].expenses.push_back(exp);
            trips[tidx].total += exp.total;
            for (int i=0;i<n;i++){
                totalPaid[i] += exp.paidFor[i];
                grandTotal   += exp.paidFor[i];
            }
            progressBar("Adding expense    ", BLU);
            cout << GRN << BOLD << "  Expense [" << exp.category << "] added to \""
                 << trips[tidx].name << "\"!"
                 << "  Trip total: " << rupee(trips[tidx].total) << RST << "\n";
            break;
        }

        // ── VIEW HISTORY ──────────────────────────
        case 3: {
            cout << "\n";
            sep('=', BLU);
            cprint("  TRIP HISTORY  ", BLU);
            if (trips.empty()) {
                cout << GRY << centre("No trips recorded yet.") << RST << "\n";
            } else {
                for (int t=0;t<(int)trips.size();t++) {
                    sep('-', GRY);
                    cout << YLW << BOLD << "  TRIP #" << t+1 << "  " << trips[t].name
                         << "  (" << rupee(trips[t].total) << ")\n" << RST;
                    for (int e=0;e<(int)trips[t].expenses.size();e++) {
                        const Expense& ex = trips[t].expenses[e];
                        cout << CYN << "   [" << ex.category << "]"
                             << GRY << " " << ex.note
                             << WHT << "  " << rupee(ex.total) << RST << "\n";
                        for (int i=0;i<n;i++) {
                            if (ex.consumed[i] > 0 || ex.paidFor[i] > 0) {
                                cout << GRY << "      |- " << left << setw(12) << names[i];
                                if (ex.consumed[i] > 0)
                                    cout << WHT << "consumed " << rupee(ex.consumed[i]);
                                if (ex.paidFor[i] > 0)
                                    cout << GRN << "  paid " << rupee(ex.paidFor[i]);
                                cout << RST << "\n";
                            }
                        }
                    }
                }
                sep('-', GRY);
                cout << YLW << BOLD << "  GRAND TOTAL : " << rupee(grandTotal) << RST << "\n";
            }
            sep('=', BLU);
            break;
        }

        // ── PRINT RECEIPT ──────────────────────────
        case 4: {
            if (trips.empty() || grandTotal == 0) {
                cout << RED << "  Add at least one expense first!\n" << RST; break;
            }
            progressBar("Generating receipt", MAG);
            printReceipt(groupName, names, totalPaid, trips, grandTotal, n);
            break;
        }

        // ── GROUP STATS ──────────────────────────
        case 5: {
            if (grandTotal == 0) {
                cout << RED << "  No data yet!\n" << RST; break;
            }
            showStats(names, totalPaid, trips, grandTotal, n, groupName);
            break;
        }

        // ── RESET ─────────────────────────────────
        case 6: {
            cout << RED << BOLD << "\n  WARNING: This will erase ALL data!\n" << RST;
            cout << WHT << "  Confirm reset? (y/n): " << YLW;
            char c; cin >> c; cout << RST;
            if (c == 'y' || c == 'Y') {
                trips.clear();
                fill(totalPaid.begin(), totalPaid.end(), 0.0);
                grandTotal = 0;
                progressBar("Clearing ledger   ", RED);
                cout << GRN << "  All data cleared!\n" << RST;
            } else {
                cout << GRY << "  Reset cancelled.\n" << RST;
            }
            break;
        }

        case 7:
            cout << "\n" << MAG << BOLD;
            sep('=', MAG);
            cprint("Thank you for using BillSplit!", MAG);
            cprint("(c) 2025 Mini Project  |  All rights reserved", GRY);
            sep('=', MAG);
            cout << RST << "\n";
            break;

        default:
            cout << RED << "  Invalid choice! Enter 1-7.\n" << RST;
        }

    } while (choice != 7);

    cout << GRN << BOLD << "\n  Code Execution Successful!\n\n" << RST;
    return 0;
}