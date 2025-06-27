# Perbandingan Implementasi B+ Tree dan Hash Map di Database Sederhana

## Daftar Isi
1. [Deskripsi Proyek](#deskripsi-proyek)  
2. [Kompilasi dan Jalankan](#kompilasi-dan-jalankan)  
3. [Overview Implementasi](#overview-implementasi)  
   - [Pengukuran Memori](#pengukuran-memori)  
   - [Metode Pengujian](#metode-pengujian)  
4. [Detail Kode](#detail-kode)  
   - [1. B+ Tree](#1-b-tree)  
     - [Struktur Node](#struktur-node)  
     - [Operasi Dasar](#operasi-dasar)  
       - [Insert](#insert)  
       - [Search](#search)  
       - [Range Search](#range-search)  
       - [Update](#update)  
       - [Remove](#remove)  
   - [2. Hash Map](#2-hash-map)  
     - [Operasi Dasar](#operasi-dasar-1)  
       - [Insert & Exact Search](#insert--exact-search)  
       - [Range Search (Brute-force)](#range-search-brute-force)  
       - [Update & Delete](#update--delete)  
5. [Pengujian dan Hasil](#pengujian-dan-hasil)  
6. [Kesimpulan](#kesimpulan)  
7. [Referensi](#referensi)  

---

## Deskripsi Proyek
Proyek ini membandingkan dua struktur data populer—**B+ Tree** dan **Hash Map**—dalam konteks operasi dasar database (insert, exact search, range search, update, delete). Setiap struktur diuji pada ukuran input `n = 100`, `500`, dan `1 000` elemen acak.

---

## Kompilasi dan Jalankan

1. **Kompilasi**  
   ```bash
   g++ compare_bptree_hashmap.cpp -o compare.exe -std=c++17 -lpsapi
   ```

2. **Jalankan**  
   - Untuk B+ Tree:  
     ```bash
     ./compare.exe bptree
     ```
   - Untuk Hash Map:  
     ```bash
     ./compare.exe hashmap
     ```

---

## Overview Implementasi

### Pengukuran Memori
- Fungsi `getCurrentRSS()` memanfaatkan Windows API (`GetProcessMemoryInfo`) untuk mendapatkan **Working Set Size** (RSS) proses saat ini.  
- Selisih memori sebelum dan sesudah operasi `insert` dilaporkan dalam KB.

### Metode Pengujian
1. **Insert** sebanyak `n` data unik.  
2. **Exact Search**: cari kembali setiap elemen yang telah di-insert.  
3. **Range Search**:   
   - Bangkitkan `QUERY_RANGE` (100) pasang range acak `[a, b]`.   
   - Cari semua pasangan key–value dalam rentang itu.  
4. **Update**: ubah setiap value dari `v` menjadi `v + 1`.  
5. **Delete**: hapus setiap key.

Waktu tiap operasi diukur dengan `std::chrono::high_resolution_clock`.

---

## Detail Kode

### 1. B+ Tree

#### Struktur Node
```cpp
struct Node {
    bool leaf;                      // Apakah node ini leaf
    vector<int> keys;               // Daftar kunci
    vector<Node*> children;         // Pointer ke anak (non-leaf)
    vector<int> values;             // Nilai (hanya untuk leaf)
    Node* next;                     // Pointer ke sibling kanan (leaf)
    Node(bool leaf): leaf(leaf), next(nullptr) {}
};
```
- **order**: derajat `m` (maks jumlah key per node), didefinisikan lewat macro `ORDER`.

#### Operasi Dasar

##### Insert
1. **insertInternal**:  
   - Jika leaf: sisipkan key di posisi terurut, split jika penuh.  
   - Jika internal: panggil rekursif, lalu mungkin split anak dan masukkan promoted key.
2. **Split Logic**:  
   - Leaf: bagi di tengah, promote key pertama sibling baru.  
   - Internal: bagi di tengah, promote satu key ke parent.

##### Search
```cpp
int search(int key);
```
- Turun dari root ke leaf mengikuti `upper_bound`.  
- Di leaf, cari dengan `lower_bound`.  
- Kembalikan `INT_MIN` jika tidak ditemukan.

##### Range Search
```cpp
vector<pair<int,int>> rangeSearch(int low, int high);
```
- Cari leaf pertama yang mungkin mengandung `low`.  
- Jelajah sibling melalui `next` hingga melewati `high`.

##### Update
```cpp
bool update(int key, int newValue);
```
- Mirip `search`, tetapi jika ditemukan, set `values[idx] = newValue`.

##### Remove
```cpp
bool remove(int key);
```
- Hanya menghapus key/value di leaf tanpa rebalancing (berpotensi underflow).

---

### 2. Hash Map

#### Operasi Dasar

##### Insert & Exact Search
- Menggunakan `std::unordered_map<int,int> mp;`

```cpp
// Insert
for (int v: data) mp[v] = v;

// Exact search
for (int v: data) {
    volatile int x = mp[v];
}
```

##### Range Search (Brute-force)
```cpp
for (auto &r: ranges) {
    for (auto &p: mp) {
        if (p.first >= r.first && p.first <= r.second) {
            volatile int y = p.second;
        }
    }
}
```
- Setiap query melintasi seluruh map.

##### Update & Delete
```cpp
// Update
for (int v: data) mp[v] = v + 1;

// Delete
for (int v: data) mp.erase(v);
```

---

## Pengujian dan Hasil

Saat dijalankan, program mencetak untuk tiap `n`:
```
B+ Tree with n items:
  Insert time: ... ms, Memory delta: ... KB
  Exact search time: ... ms
  Range search time (100 queries): ... ms
  Update time: ... ms
  Delete time: ... ms

Hash Map with n items:
  Insert time: ... ms, Memory delta: ... KB
  Exact search time: ... ms
  Range search time (100 queries): ... ms
  Update time: ... ms
  Delete time: ... ms
```
- **B+ Tree** cenderung lebih cepat pada range search karena traversal di leaf yang terurut.  
- **Hash Map** unggul di exact search (O(1) rata-rata) dan insert.  
- Banyak trade-off: B+ Tree lebih terstruktur untuk rentang, Hash Map lebih sederhana untuk akses acak.

---

## Kesimpulan
- **B+ Tree** cocok untuk aplikasi database yang banyak menjalankan **range queries** dan membutuhkan struktur terurut di disk/memori.  
- **Hash Map** ideal untuk **key–value lookup** cepat tanpa kebutuhan rentang, dan memiliki implementasi yang lebih ringkas.  
- Pilih struktur data sesuai pola akses utama di aplikasi Anda.

---

## Referensi
- Dokumentasi [C++ STL `unordered_map`](https://en.cppreference.com/w/cpp/container/unordered_map)  
- Teori dan implementasi B+ Tree, berbagai literatur struktur data.

## Dokumentasi
![Screenshot 2025-06-25 124340](https://github.com/user-attachments/assets/4e952978-928e-498b-b39b-5ffd1f9438c7)
![Screenshot 2025-06-25 121725](https://github.com/user-attachments/assets/aec6a54f-44f0-458c-8250-bd4e6970f8e2)
