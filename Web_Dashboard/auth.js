// auth.js

// Fungsi untuk Proteksi Halaman
(function() {
    // Cek apakah pengguna sudah login berdasarkan sessionStorage
    if (sessionStorage.getItem("loggedIn") !== "true") {
        // Dapatkan halaman saat ini
        const currentPage = window.location.pathname.split('/').pop();
        
        // Izinkan akses ke index.html dan login.html tanpa login
        if (currentPage !== 'index.html' && currentPage !== 'login.html') {
            // Jika belum login dan tidak di halaman index/login, redirect ke login.html
            window.location.href = "login.html";
        }
    }
})();

// Fungsi untuk Logout
function logout() {
    sessionStorage.removeItem('loggedIn'); // Hapus status login dari sessionStorage
    window.location.href = 'login.html'; // Redirect ke halaman login
}

// Tambahkan event listener untuk tombol logout di sidebar setelah DOM dimuat
document.addEventListener('DOMContentLoaded', () => {
    const logoutButton = document.getElementById('logoutBtnSidebar');
    if (logoutButton) {
        logoutButton.addEventListener('click', function(e) {
            e.preventDefault(); // Mencegah aksi default link
            logout();
        });
    }
});