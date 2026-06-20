#include <iostream>
#include <iomanip>
#include <limits>   // untuk numeric_limits, dipakai untuk membersihkan sisa buffer input
#include <conio.h>
using namespace std;

// ===================== KONSTANTA =====================
const int MAX_USER    = 30;
const int MAX_FILM    = 20;
const int MAX_JADWAL  = 30;
const int MAX_PROMO   = 10;
const int MAX_TRX     = 50;
const int TOTAL_KURSI = 40;
const int MAX_STUDIO  = 3;

// ===================== STRUCT =====================
struct User {
    string username;
    string password;
};

struct Film {
    string id, judul, genre, rating;
    int    durasi, stok, terjual;
    float  harga;
};

struct Jadwal {
    string id, tanggal, jam, jamSelesai;
    int    film, studio, mulai, selesai;
    bool   kursi[TOTAL_KURSI];
};

struct Promo {
    string kode;
    int    diskon;
    bool   aktif;
};

struct Transaksi {
    string id, va, kodeTiket;
    int    user, jadwal, jumlahTiket;
    int    kursi[TOTAL_KURSI];
    float  total, diskon;
};

// ===================== DATA GLOBAL =====================
User       user[MAX_USER];
Film       film[MAX_FILM];
Jadwal     jadwal[MAX_JADWAL];
Promo      promo[MAX_PROMO];
Transaksi  transaksi[MAX_TRX];

int  jmlUser = 0, jmlFilm = 0, jmlJadwal = 0, jmlPromo = 0, jmlTrx = 0;
int  userAktif = -1;

string adminUsn = "admin";
string adminPsw = "admin123";

// Variabel sementara untuk proses reservasi -> pembayaran
int tmpFilm = -1, tmpJadwal = -1, tmpJumlahTiket = 0;
int tmpKursi[TOTAL_KURSI];

// ===================== UTILITAS =====================

// Mencetak garis pemisah
void garis() {
    for (int i = 0; i < 60; i++) cout << "=";
    cout << endl;
}

// Membersihkan layar dan mencetak header program
void header(string judul) {
    system("cls");
    garis();
    cout << "        CinemaXXV - Reservasi Tiket Bioskop\n";
    cout << "              " << judul << endl;
    garis();
}

// Menunggu input tombol dari pengguna sebelum lanjut
void pause() {
    cout << "\nTekan sembarang tombol...";
    getch();
}

// Mengubah semua huruf string menjadi huruf kecil
string toLower(string s) {
    for (int i = 0; i < (int)s.length(); i++)
        if (s[i] >= 'A' && s[i] <= 'Z') s[i] += 32;
    return s;
}

// Mengubah nomor kursi (0-39) menjadi label "A1".."E8"
string namaKursi(int nk) {
    return string(1, char('A' + nk / 8)) + to_string(nk % 8 + 1);
}

// ===================== INPUT INTEGER TERVALIDASI =====================

// Membaca input integer dari pengguna dengan validasi ketat:
// - Menolak input yang mengandung karakter non-digit (contoh: "15a", "1b")
// - Menolak input di luar rentang [min, max]
// - Mengembalikan nilai integer yang valid
int inputInt(string prompt, int minVal, int maxVal) {
    string buf;
    int    val;
    bool   valid;
    do {
        valid = true;
        cout << prompt;
        cin  >> buf;

        // Cek setiap karakter harus digit
        for (int i = 0; i < (int)buf.length(); i++) {
            if (!isdigit(buf[i])) { valid = false; break; }
        }

        if (!valid || buf.empty()) {
            cout << "Input tidak valid, masukkan angka.\n";
            valid = false;
            continue;
        }

        val = stoi(buf);
        if (val < minVal || val > maxVal) {
            cout << "Input harus antara " << minVal << " - " << maxVal << ".\n";
            valid = false;
        }
    } while (!valid);
    return val;
}

// ===================== MENU INTERAKTIF =====================

// Menampilkan menu dengan navigasi tombol panah dan Enter
// Mengembalikan index pilihan yang dipilih (0, 1, 2, ...)
int pilihMenu(string judul, string opsi[], int jmlOpsi) {
    int pilih = 0, tombol;
    do {
        header(judul);
        cout << "\nGunakan panah Atas/Bawah, Enter untuk pilih\n";
        garis();
        for (int i = 0; i < jmlOpsi; i++)
            cout << (i == pilih ? " >> " : "    ") << opsi[i] << endl;
        garis();

        tombol = getch();
        if (tombol == 224) {        // kode awal tombol panah
            tombol = getch();
            if (tombol == 72)       // panah atas
                pilih = (pilih - 1 + jmlOpsi) % jmlOpsi;
            else if (tombol == 80)  // panah bawah
                pilih = (pilih + 1) % jmlOpsi;
        }
    } while (tombol != 13);         // 13 = Enter
    return pilih;
}

// ===================== INPUT TANGGAL & JAM =====================

// Meminta input tanggal dengan validasi, mengembalikan string "DD/MM/YYYY"
string inputTanggal() {
    int tgl = inputInt("Tanggal (1-31)    : ", 1, 31);
    int bln = inputInt("Bulan (1-12)      : ", 1, 12);
    int thn = inputInt("Tahun (2026-2030) : ", 2026, 2030);

    string sTgl = (tgl < 10 ? "0" : "") + to_string(tgl);
    string sBln = (bln < 10 ? "0" : "") + to_string(bln);
    return sTgl + "/" + sBln + "/" + to_string(thn);
}

// Meminta input jam dengan validasi, mengembalikan string "HH.MM"
string inputJam() {
    int jamH = inputInt("Jam (0-23)   : ", 0, 23);
    int jamM = inputInt("Menit (0-59) : ", 0, 59);

    return (jamH < 10 ? "0" : "") + to_string(jamH) + "." +
           (jamM < 10 ? "0" : "") + to_string(jamM);
}

// Mengubah string "HH.MM" menjadi total menit, contoh: "18.00" -> 1080
int jamKeMenit(string jam) {
    for (int i = 0; i < (int)jam.length(); i++)
        if (jam[i] == ':') jam[i] = '.';
    int pos = jam.find('.');
    return stoi(jam.substr(0, pos)) * 60 + stoi(jam.substr(pos + 1));
}

// Mengubah total menit menjadi string "HH.MM", contoh: 1080 -> "18.00"
string menitKeJam(int menit) {
    int jamH = (menit / 60) % 24;
    int jamM = menit % 60;
    return (jamH < 10 ? "0" : "") + to_string(jamH) + "." +
           (jamM < 10 ? "0" : "") + to_string(jamM);
}

// ===================== SEARCHING =====================

// Mencari index user berdasarkan username, mengembalikan -1 jika tidak ada
int cariUser(string usn) {
    for (int i = 0; i < jmlUser; i++)
        if (toLower(user[i].username) == toLower(usn)) return i;
    return -1;
}

// Mencari index film berdasarkan ID, mengembalikan -1 jika tidak ada
int cariFilm(string id) {
    for (int i = 0; i < jmlFilm; i++)
        if (toLower(film[i].id) == toLower(id)) return i;
    return -1;
}

// Mencari index jadwal berdasarkan ID, mengembalikan -1 jika tidak ada
int cariJadwal(string id) {
    for (int i = 0; i < jmlJadwal; i++)
        if (toLower(jadwal[i].id) == toLower(id)) return i;
    return -1;
}

// Mencari index promo berdasarkan kode, mengembalikan -1 jika tidak ada
int cariPromo(string kode) {
    for (int i = 0; i < jmlPromo; i++)
        if (toLower(promo[i].kode) == toLower(kode)) return i;
    return -1;
}

// ===================== CEK BENTROK JADWAL =====================

// Mengecek apakah jadwal baru bentrok dengan jadwal yang sudah ada.
// Parameter kecuali diisi index jadwal yang tidak dibandingkan (saat edit),
// isi -1 jika tidak ada yang dikecualikan.
bool cekBentrok(int studio, string tanggal, int mulaiBaru, int selesaiBaru, int kecuali) {
    for (int i = 0; i < jmlJadwal; i++) {
        if (i == kecuali) continue;
        if (jadwal[i].studio == studio && jadwal[i].tanggal == tanggal) {
            if (mulaiBaru < jadwal[i].selesai && jadwal[i].mulai < selesaiBaru)
                return true; // bentrok
        }
    }
    return false;
}

// ===================== POINTER =====================

// Menghitung total pendapatan dan total tiket terjual dari semua transaksi
void hitungTotal(float *pendapatan, int *tiket) {
    *pendapatan = 0;
    *tiket      = 0;
    for (int i = 0; i < jmlTrx; i++) {
        *pendapatan += transaksi[i].total;
        *tiket      += transaksi[i].jumlahTiket;
    }
}

// ===================== SIGN UP & SIGN IN =====================

void signUp() {
    header("SIGN UP");
    if (jmlUser >= MAX_USER) { cout << "Data user penuh.\n"; pause(); return; }

    string usn, psw;

    // cin.clear/ignore + getline mencegah infinite loop saat Enter ditekan langsung
    do {
        cout << "\nUsername : ";
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        getline(cin, usn);
        if (usn.length() == 0) cout << "Username tidak boleh kosong.\n";
    } while (usn.length() == 0);

    if (cariUser(usn) != -1) { cout << "Username sudah digunakan.\n"; pause(); return; }

    do {
        cout << "Password : ";
        getline(cin, psw);
        if (psw.length() == 0) cout << "Password tidak boleh kosong.\n";
    } while (psw.length() == 0);

    user[jmlUser].username = usn;
    user[jmlUser].password = psw;
    jmlUser++;

    cout << "\nAkun berhasil dibuat.\n";
    pause();
}

// Mengembalikan 0 = gagal, 1 = admin, 2 = customer
int signIn() {
    header("SIGN IN");
    string usn, psw;
    cout << "\nUsername : "; cin >> usn;
    cout << "Password : "; cin >> psw;

    if (usn.length() == 0 || psw.length() == 0) {
        cout << "\nUsername/password tidak boleh kosong.\n";
        pause(); return 0;
    }

    // Cek login admin
    if (usn == adminUsn && psw == adminPsw) {
        cout << "\nLogin admin berhasil.\n";
        pause(); return 1;
    }

    // Cek login customer
    int idx = cariUser(usn);
    if (idx != -1 && user[idx].password == psw) {
        userAktif = idx;
        cout << "\nSelamat datang, " << user[idx].username << "!\n";
        pause(); return 2;
    }

    cout << "\nUsername atau password salah.\n";
    pause(); return 0;
}

// ===================== MANAJEMEN FILM =====================

// Menampilkan seluruh data film dalam format tabel
void tampilFilm() {
    if (jmlFilm == 0) { cout << "\nBelum ada film.\n"; return; }
    garis();
    cout << left << setw(8)  << "ID"
                 << setw(20) << "Judul"
                 << setw(10) << "Genre"
                 << setw(8)  << "Durasi"
                 << setw(8)  << "Usia"
                 << "Harga" << endl;
    garis();
    for (int i = 0; i < jmlFilm; i++)
        cout << left << setw(8)  << film[i].id
                     << setw(20) << film[i].judul
                     << setw(10) << film[i].genre
                     << setw(8)  << film[i].durasi
                     << setw(8)  << film[i].rating
                     << film[i].harga << endl;
    garis();
}

string pilihGenre(string judul) {
    string genreList[5] = {"Action", "Horror", "Comedy", "Drama", "Animasi"};
    return genreList[pilihMenu(judul, genreList, 5)];
}
string pilihRating(string judul) {
    string ratingList[4] = {"SU", "13+", "17+", "21+"};
    return ratingList[pilihMenu(judul, ratingList, 4)];
}

void tambahFilm() {
    header("TAMBAH FILM");
    if (jmlFilm >= MAX_FILM) { cout << "\nData film penuh.\n"; pause(); return; }

    Film f;
    cout << "\nID Film    : "; cin >> f.id;
    if (cariFilm(f.id) != -1) { cout << "ID sudah ada.\n"; pause(); return; }

    cin.ignore();
    cout << "Judul Film : "; getline(cin, f.judul);

    f.genre  = pilihGenre("PILIH GENRE");
    f.rating = pilihRating("PILIH RATING USIA");

    header("TAMBAH FILM");
    cout << "\nID Film    : " << f.id    << endl;
    cout << "Judul Film : " << f.judul  << endl;
    cout << "Genre      : " << f.genre  << endl;
    cout << "Rating     : " << f.rating << endl;

    f.durasi = inputInt("\nDurasi (40-360 menit)      : ", 40, 360);
    f.harga  = (float)inputInt("Harga Tiket (25000-300000) : ", 25000, 300000);

    f.stok    = TOTAL_KURSI; // stok otomatis mengikuti jumlah kursi
    f.terjual = 0;
    film[jmlFilm++] = f;

    cout << "\nFilm berhasil ditambahkan.\n";
    pause();
}

void editFilm() {
    header("EDIT FILM");
    tampilFilm();
    if (jmlFilm == 0) { pause(); return; }

    string id;
    cout << "\nMasukkan ID Film : "; cin >> id;
    int idx = cariFilm(id);
    if (idx == -1) { cout << "Film tidak ditemukan.\n"; pause(); return; }

    cin.ignore();
    cout << "Judul Baru : "; getline(cin, film[idx].judul);

    film[idx].genre  = pilihGenre("PILIH GENRE BARU");
    film[idx].rating = pilihRating("PILIH RATING USIA BARU");

    header("EDIT FILM");
    film[idx].durasi = inputInt("\nDurasi Baru (40-360 menit)      : ", 40, 360);
    film[idx].harga  = (float)inputInt("Harga Baru (25000-300000) : ", 25000, 300000);

    film[idx].stok = TOTAL_KURSI;
    cout << "\nFilm berhasil diubah.\n";
    pause();
}

void hapusFilm() {
    header("HAPUS FILM");
    tampilFilm();
    if (jmlFilm == 0) { pause(); return; }

    string id;
    cout << "\nMasukkan ID Film : "; cin >> id;
    int idx = cariFilm(id);
    if (idx == -1) { cout << "Film tidak ditemukan.\n"; pause(); return; }

    // Geser elemen ke kiri untuk menghapus
    for (int i = idx; i < jmlFilm - 1; i++) film[i] = film[i + 1];
    jmlFilm--;

    cout << "\nFilm berhasil dihapus.\n";
    pause();
}

// Mengurutkan film berdasarkan jumlah terjual (descending) dengan selection sort
void urutFilmTerjual() {
    for (int i = 0; i < jmlFilm - 1; i++)
        for (int j = i + 1; j < jmlFilm; j++)
            if (film[j].terjual > film[i].terjual) {
                Film tmp = film[i]; film[i] = film[j]; film[j] = tmp;
            }
}

void menuFilm() {
    string opsi[5] = {"Tambah Film", "Edit Film", "Hapus Film", "Lihat Film", "Kembali"};
    int pilih;
    do {
        pilih = pilihMenu("MANAJEMEN FILM", opsi, 5);
        switch (pilih) {
            case 0: tambahFilm(); break;
            case 1: editFilm();   break;
            case 2: hapusFilm();  break;
            case 3:
                header("DAFTAR FILM");
                urutFilmTerjual();
                tampilFilm();
                pause();
                break;
        }
    } while (pilih != 4);
}

// ===================== MANAJEMEN JADWAL =====================

// Menampilkan seluruh data jadwal dalam format tabel
void tampilJadwal() {
    if (jmlJadwal == 0) { cout << "\nBelum ada jadwal.\n"; return; }
    garis();
    cout << left << setw(8)  << "ID"
                 << setw(20) << "Film"
                 << setw(8)  << "Studio"
                 << setw(15) << "Tanggal"
                 << "Jam" << endl;
    garis();
    for (int i = 0; i < jmlJadwal; i++)
        cout << left << setw(8)  << jadwal[i].id
                     << setw(20) << film[jadwal[i].film].judul
                     << setw(8)  << jadwal[i].studio
                     << setw(15) << jadwal[i].tanggal
                     << jadwal[i].jam << " - " << jadwal[i].jamSelesai << endl;
    garis();
}

void tambahJadwal() {
    header("TAMBAH JADWAL");
    if (jmlFilm == 0) { cout << "\nTambahkan film terlebih dahulu.\n"; pause(); return; }

    tampilFilm();
    Jadwal j;

    // Validasi ID jadwal tidak boleh duplikat
    do {
        cout << "\nID Jadwal : "; cin >> j.id;
        if (cariJadwal(j.id) != -1) cout << "ID jadwal sudah ada, gunakan ID lain.\n";
    } while (cariJadwal(j.id) != -1);

    string idFilm;
    cout << "ID Film   : "; cin >> idFilm;
    int idxFilm = cariFilm(idFilm);
    if (idxFilm == -1) { cout << "Film tidak ditemukan.\n"; pause(); return; }
    j.film = idxFilm;

    j.studio = inputInt("Studio (1-" + to_string(MAX_STUDIO) + ") : ", 1, MAX_STUDIO);

    cout << "\n-- Masukkan Tanggal Tayang --\n"; j.tanggal = inputTanggal();
    cout << "\n-- Masukkan Jam Mulai --\n";     j.jam     = inputJam();

    // Hitung jam selesai: durasi film + 15 menit cleaning room
    j.mulai    = jamKeMenit(j.jam);
    j.selesai  = j.mulai + film[idxFilm].durasi + 15;
    j.jamSelesai = menitKeJam(j.selesai);

    if (cekBentrok(j.studio, j.tanggal, j.mulai, j.selesai, -1)) {
        cout << "\nJadwal bentrok dengan jadwal yang sudah ada.\n";
        pause(); return;
    }

    for (int i = 0; i < TOTAL_KURSI; i++) j.kursi[i] = false;
    jadwal[jmlJadwal++] = j;

    cout << "\nJadwal berhasil ditambahkan (Jam " << j.jam << " - " << j.jamSelesai << ").\n";
    pause();
}

void editJadwal() {
    header("EDIT JADWAL");
    tampilJadwal();

    string id;
    cout << "\nMasukkan ID Jadwal : "; cin >> id;
    int idx = cariJadwal(id);
    if (idx == -1) { cout << "Jadwal tidak ditemukan.\n"; pause(); return; }

    int studioBaru = inputInt("Studio Baru (1-" + to_string(MAX_STUDIO) + ") : ", 1, MAX_STUDIO);

    cout << "\n-- Masukkan Tanggal Baru --\n"; string tanggalBaru = inputTanggal();
    cout << "\n-- Masukkan Jam Mulai Baru --\n"; string jamBaru = inputJam();

    int mulaiBaru   = jamKeMenit(jamBaru);
    int selesaiBaru = mulaiBaru + film[jadwal[idx].film].durasi + 15;

    // Kecualikan jadwal ini sendiri saat cek bentrok (parameter idx)
    if (cekBentrok(studioBaru, tanggalBaru, mulaiBaru, selesaiBaru, idx)) {
        cout << "\nJadwal bentrok dengan jadwal yang sudah ada.\n";
        pause(); return;
    }

    jadwal[idx].studio     = studioBaru;
    jadwal[idx].tanggal    = tanggalBaru;
    jadwal[idx].jam        = jamBaru;
    jadwal[idx].mulai      = mulaiBaru;
    jadwal[idx].selesai    = selesaiBaru;
    jadwal[idx].jamSelesai = menitKeJam(selesaiBaru);

    cout << "\nJadwal berhasil diubah.\n";
    pause();
}

void hapusJadwal() {
    header("HAPUS JADWAL");
    tampilJadwal();

    string id;
    cout << "\nMasukkan ID Jadwal : "; cin >> id;
    int idx = cariJadwal(id);
    if (idx == -1) { cout << "Jadwal tidak ditemukan.\n"; pause(); return; }

    for (int i = idx; i < jmlJadwal - 1; i++) jadwal[i] = jadwal[i + 1];
    jmlJadwal--;

    cout << "\nJadwal berhasil dihapus.\n";
    pause();
}

void menuJadwal() {
    string opsi[5] = {"Tambah Jadwal", "Edit Jadwal", "Hapus Jadwal", "Lihat Jadwal", "Kembali"};
    int pilih;
    do {
        pilih = pilihMenu("MANAJEMEN JADWAL", opsi, 5);
        switch (pilih) {
            case 0: tambahJadwal(); break;
            case 1: editJadwal();   break;
            case 2: hapusJadwal();  break;
            case 3:
                header("DAFTAR JADWAL");
                tampilJadwal();
                pause();
                break;
        }
    } while (pilih != 4);
}

// ===================== MANAJEMEN PROMO =====================

// Menampilkan seluruh data promo dalam format tabel
void tampilPromo() {
    if (jmlPromo == 0) { cout << "\nBelum ada promo.\n"; return; }
    garis();
    cout << left << setw(15) << "Kode" << setw(10) << "Diskon" << "Status" << endl;
    garis();
    for (int i = 0; i < jmlPromo; i++)
        cout << left << setw(15) << promo[i].kode
                     << setw(10) << promo[i].diskon
                     << (promo[i].aktif ? "Aktif" : "Nonaktif") << endl;
    garis();
}

// Membaca kode promo satu baris penuh via getline, lalu validasi:
// - Tidak boleh kosong
// - Tidak boleh mengandung spasi (contoh: "DISKON 20" ditolak dan diminta ulang)
// cin.ignore dipanggil SEKALI sebelum loop untuk membersihkan sisa '\n'
// dari input sebelumnya (misalnya setelah cin >>). Jika diletakkan di
// dalam loop, getline di iterasi ke-2 dst langsung dapat "" karena
// cin.ignore membuang karakter yang baru saja diketik user.
string inputKodePromo() {
    string kode;
    bool   valid;
    cin.clear();
    cin.ignore(numeric_limits<streamsize>::max(), '\n'); // bersihkan buffer sekali di awal
    do {
        valid = true;
        cout << "  (tanpa spasi, contoh: DISKON20)\n";
        cout << "Kode Promo : ";
        getline(cin, kode);

        if (kode.empty()) {
            cout << "Kode tidak boleh kosong.\n";
            valid = false;
            continue;
        }

        // Cek apakah ada spasi di dalam input
        for (int i = 0; i < (int)kode.length(); i++) {
            if (kode[i] == ' ') {
                cout << "Kode promo tidak boleh mengandung spasi.\n";
                valid = false;
                break;
            }
        }
    } while (!valid);
    return kode;
}

void tambahPromo() {
    header("TAMBAH PROMO");
    if (jmlPromo >= MAX_PROMO) { cout << "\nData promo penuh.\n"; pause(); return; }

    Promo p;
    cout << "\n";
    p.kode = inputKodePromo();
    if (cariPromo(p.kode) != -1) { cout << "Kode promo sudah ada.\n"; pause(); return; }

    p.diskon = inputInt("Diskon (%) : ", 1, 100);
    p.aktif  = true;
    promo[jmlPromo++] = p;

    cout << "\nPromo berhasil ditambahkan.\n";
    pause();
}

void editPromo() {
    header("EDIT PROMO");
    tampilPromo();

    cout << "\n";
    string kode = inputKodePromo();
    int idx = cariPromo(kode);
    if (idx == -1) { cout << "Promo tidak ditemukan.\n"; pause(); return; }

    promo[idx].diskon = inputInt("Diskon Baru                  : ", 1, 100);

    // Dibaca sebagai string agar "1a" tidak lolos seperti saat pakai cin >> int
    string inputStatus;
    do {
        cout << "Status (1=aktif, 0=nonaktif) : "; cin >> inputStatus;
        if (inputStatus != "0" && inputStatus != "1")
            cout << "Input tidak valid, masukkan 0 atau 1.\n";
    } while (inputStatus != "0" && inputStatus != "1");
    promo[idx].aktif = (inputStatus == "1");

    cout << "\nPromo berhasil diubah.\n";
    pause();
}

void hapusPromo() {
    header("HAPUS PROMO");
    tampilPromo();

    cout << "\n";
    string kode = inputKodePromo();
    int idx = cariPromo(kode);
    if (idx == -1) { cout << "Promo tidak ditemukan.\n"; pause(); return; }

    for (int i = idx; i < jmlPromo - 1; i++) promo[i] = promo[i + 1];
    jmlPromo--;

    cout << "\nPromo berhasil dihapus.\n";
    pause();
}

void menuPromo() {
    string opsi[5] = {"Tambah Promo", "Edit Promo", "Hapus Promo", "Lihat Promo", "Kembali"};
    int pilih;
    do {
        pilih = pilihMenu("MANAJEMEN PROMO", opsi, 5);
        switch (pilih) {
            case 0: tambahPromo(); break;
            case 1: editPromo();   break;
            case 2: hapusPromo();  break;
            case 3:
                header("DAFTAR PROMO");
                tampilPromo();
                pause();
                break;
        }
    } while (pilih != 4);
}

// ===================== LAPORAN =====================

void laporanPenjualan() {
    header("LAPORAN PENJUALAN");
    float pendapatan;
    int tiket;
    hitungTotal(&pendapatan, &tiket);

    cout << "\nTotal Tiket Terjual : " << tiket      << endl;
    cout << "Total Transaksi     : " << jmlTrx      << endl;
    cout << "Total Pendapatan    : Rp" << pendapatan << endl;
    garis();

    for (int i = 0; i < jmlTrx; i++)
        cout << transaksi[i].id << "  Rp" << transaksi[i].total << endl;

    pause();
}

// ===================== MENU ADMIN =====================

void menuAdmin() {
    string opsi[5] = {"Manajemen Film", "Manajemen Jadwal", "Manajemen Promo", "Laporan Penjualan", "Logout"};
    int pilih;
    do {
        pilih = pilihMenu("MENU ADMIN", opsi, 5);
        switch (pilih) {
            case 0: menuFilm();          break;
            case 1: menuJadwal();        break;
            case 2: menuPromo();         break;
            case 3: laporanPenjualan();  break;
        }
    } while (pilih != 4);
}

// ===================== FITUR CUSTOMER =====================

void berandaCustomer() {
    header("BERANDA CUSTOMER");
    cout << "\nSelamat datang, " << user[userAktif].username << "!\n";
    cout << "\nDaftar Film\n";
    tampilFilm();
    pause();
}

// Menampilkan layout kursi bioskop dengan label baris (A-E) dan kolom (1-8)
void tampilKursi(int idx) {
    cout << "\n          [ LAYAR BIOSKOP ]\n\n";
    cout << "     ";
    for (int k = 1; k <= 8; k++) cout << " " << k << " ";
    cout << endl;
    garis();

    for (int baris = 0; baris < 5; baris++) {
        cout << "  " << (char)('A' + baris) << "  ";
        for (int kolom = 0; kolom < 8; kolom++) {
            int nomorKursi = baris * 8 + kolom;
            cout << (jadwal[idx].kursi[nomorKursi] ? "[X]" : "[O]");
        }
        cout << endl;
    }

    garis();
    cout << "  Keterangan: [O] Tersedia   [X] Terisi\n";
    garis();
}

void reservasiTiket() {
    header("RESERVASI TIKET");
    tampilJadwal();

    string id;
    cout << "\nID Jadwal : "; cin >> id;
    int idx = cariJadwal(id);
    if (idx == -1) { cout << "Jadwal tidak ditemukan.\n"; pause(); return; }

    tmpJadwal = idx;
    tmpFilm   = jadwal[idx].film;

    // Hitung kursi yang masih tersedia
    int kursiTersedia = 0;
    for (int i = 0; i < TOTAL_KURSI; i++)
        if (!jadwal[idx].kursi[i]) kursiTersedia++;

    if (kursiTersedia == 0) { cout << "\nKursi sudah penuh.\n"; pause(); return; }

    tmpJumlahTiket = inputInt("Jumlah Tiket (1-" + to_string(kursiTersedia) + ") : ", 1, kursiTersedia);

    tampilKursi(idx);
    cout << "\nPilih kursi (contoh: A1, B3, E8)\n";

    for (int i = 0; i < tmpJumlahTiket; i++) {
        string inputKursi;
        int    nomorKursi;
        bool   valid;

        do {
            valid = true;
            cout << "Kursi " << i + 1 << " : "; cin >> inputKursi;

            // Panjang harus tepat 2 karakter (1 huruf + 1 angka), contoh "A2B" ditolak
            if (inputKursi.length() != 2) {
                cout << "Format tidak valid, contoh: A1\n"; valid = false; continue;
            }

            char baris = inputKursi[0];
            if (baris >= 'a' && baris <= 'z') baris -= 32;

            if (baris < 'A' || baris > 'E') {
                cout << "Baris tidak valid, gunakan A-E.\n"; valid = false; continue;
            }

            int kolom = isdigit(inputKursi[1]) ? stoi(inputKursi.substr(1)) : 0;
            if (kolom < 1 || kolom > 8) {
                cout << "Kolom tidak valid, gunakan 1-8.\n"; valid = false; continue;
            }

            nomorKursi = (baris - 'A') * 8 + (kolom - 1);

            if (jadwal[idx].kursi[nomorKursi]) {
                cout << "Kursi " << baris << kolom << " sudah terisi.\n"; valid = false; continue;
            }

            // Cek apakah kursi sudah dipilih di iterasi sebelumnya
            for (int k = 0; k < i && valid; k++)
                if (tmpKursi[k] - 1 == nomorKursi) {
                    cout << "Kursi " << baris << kolom << " sudah dipilih.\n"; valid = false;
                }

        } while (!valid);

        tmpKursi[i] = nomorKursi + 1; // simpan sebagai nomor 1-40
    }

    cout << "\nKursi yang dipilih:\n";
    for (int i = 0; i < tmpJumlahTiket; i++)
        cout << i + 1 << ". " << namaKursi(tmpKursi[i] - 1) << endl;

    cout << "\nReservasi berhasil.\n";
    pause();
}

// Fungsi pembuat ID unik untuk transaksi, tiket, dan virtual account
string buatIDTrx()     { return "TRX" + to_string(jmlTrx + 1001); }
string buatKodeTiket() { return "TKT" + to_string(jmlTrx + 5001); }
string buatVA()        { return "8807" + to_string(jmlTrx + 10001); }

void pembayaran() {
    header("PEMBAYARAN");

    float totalTiket = tmpJumlahTiket * film[tmpFilm].harga;
    float total      = totalTiket;

    cout << "\nFilm        : " << film[tmpFilm].judul << endl;
    cout << "Total Tiket : Rp" << totalTiket << endl;
    cout << "Total Bayar : Rp" << total << endl;

    string kode;
    cout << "\nKode Promo (- jika tidak ada) : "; cin >> kode;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    float diskon = 0;
    if (kode != "-") {
        int idx = cariPromo(kode);
        if (idx == -1)
            cout << "Kode promo tidak ditemukan, promo tidak diterapkan.\n";
        else if (!promo[idx].aktif)
            cout << "Kode promo tidak aktif, promo tidak diterapkan.\n";
        else {
            diskon = total * promo[idx].diskon / 100.0;
            total -= diskon;
            cout << "Promo berhasil diterapkan (" << promo[idx].diskon << "%).\n";
        }
    }

    cout << "\nDiskon      : Rp" << diskon << endl;
    cout << "Total Akhir : Rp" << total   << endl;
    string va = buatVA();
    cout << "\nVirtual Account : " << va << endl;
    cout << "\nPembayaran berhasil.\n";
    pause();

    // Simpan transaksi
    Transaksi &t    = transaksi[jmlTrx];
    t.id            = buatIDTrx();
    t.kodeTiket     = buatKodeTiket();
    t.va            = va;
    t.user          = userAktif;
    t.jadwal        = tmpJadwal;
    t.jumlahTiket   = tmpJumlahTiket;
    t.total         = total;
    t.diskon        = diskon;

    for (int i = 0; i < tmpJumlahTiket; i++) {
        t.kursi[i] = tmpKursi[i];
        jadwal[tmpJadwal].kursi[tmpKursi[i] - 1] = true; // tandai kursi terisi
    }

    film[tmpFilm].terjual += tmpJumlahTiket;
    jmlTrx++;

    // Reset tmpJadwal agar tidak bisa membayar ulang tanpa reservasi baru
    tmpJadwal = -1;
}

void cetakTiket() {
    header("CETAK TIKET");
    if (jmlTrx == 0) { cout << "\nBelum ada transaksi.\n"; pause(); return; }

    Transaksi &t = transaksi[jmlTrx - 1];
    Jadwal    &j = jadwal[t.jadwal];

    cout << "\nKode Tiket   : " << t.kodeTiket       << endl;
    cout << "Film         : " << film[j.film].judul << endl;
    cout << "Tanggal      : " << j.tanggal          << endl;
    cout << "Jam          : " << j.jam << " - " << j.jamSelesai << endl;
    cout << "Studio       : " << j.studio           << endl;
    cout << "Jumlah Tiket : " << t.jumlahTiket      << endl;
    cout << "Kursi        : ";

    for (int i = 0; i < t.jumlahTiket; i++) {
        cout << namaKursi(t.kursi[i] - 1);
        if (i < t.jumlahTiket - 1) cout << ", ";
    }
    cout << endl;
    pause();
}

void riwayatPemesanan() {
    header("RIWAYAT PEMESANAN");
    bool ada = false;

    for (int i = 0; i < jmlTrx; i++) {
        if (transaksi[i].user != userAktif) continue;
        ada = true;
        cout << "\nID Transaksi : " << transaksi[i].id << endl;
        cout << "Film         : " << film[jadwal[transaksi[i].jadwal].film].judul << endl;
        cout << "Jumlah Tiket : " << transaksi[i].jumlahTiket << endl;
        cout << "Total Bayar  : Rp" << transaksi[i].total << endl;
        cout << "Nomor VA     : " << transaksi[i].va << endl;
        garis();
    }

    if (!ada) cout << "\nBelum ada riwayat.\n";
    pause();
}

// ===================== MENU CUSTOMER =====================

void menuCustomer() {
    string opsi[6] = {"Beranda", "Reservasi Tiket", "Pembayaran", "Cetak Tiket", "Riwayat Pemesanan", "Logout"};
    int pilih;
    do {
        pilih = pilihMenu("MENU CUSTOMER", opsi, 6);
        switch (pilih) {
            case 0: berandaCustomer(); break;
            case 1: reservasiTiket();  break;
            case 2:
                // Cek reservasi sudah dilakukan sebelum lanjut ke pembayaran
                if (tmpJadwal == -1) {
                    header("PEMBAYARAN");
                    cout << "\nLakukan reservasi tiket terlebih dahulu.\n";
                    pause();
                } else {
                    pembayaran();
                }
                break;
            case 3: cetakTiket();        break;
            case 4: riwayatPemesanan();  break;
        }
    } while (pilih != 5); // 5 = Logout
    userAktif = -1;
    tmpJadwal = -1; // reset saat logout agar sesi berikutnya bersih
}

// ===================== MENU UTAMA =====================

void menuUtama() {
    string opsi[3] = {"Sign Up", "Sign In", "Exit"};
    int pilih;
    do {
        pilih = pilihMenu("CINEMAXXV", opsi, 3);
        if (pilih == 0) {
            signUp();
        } else if (pilih == 1) {
            int login = signIn();
            if (login == 1)      menuAdmin();
            else if (login == 2) menuCustomer();
        }
    } while (pilih != 2);
}

// ===================== MAIN =====================

int main() {
    menuUtama();
    return 0;
}
