// sidebar.js

function openNav() {
  document.getElementById("mySidebar").style.width = "250px";
  // Jika Anda ingin konten utama terdorong saat sidebar terbuka, tambahkan:
  // document.querySelector(".wrapper").style.marginLeft = "250px"; // Sesuaikan selector jika perlu
}

function closeNav() {
  document.getElementById("mySidebar").style.width = "0";
  // Jika Anda mendorong konten utama, kembalikan:
  // document.querySelector(".wrapper").style.marginLeft = "0"; // Sesuaikan selector jika perlu
}

document.addEventListener('DOMContentLoaded', () => {
    const openMenuButton = document.getElementById('openMenuBtn');
    if(openMenuButton){
        openMenuButton.addEventListener('click', openNav);
    }

    // Tombol close di dalam sidebar sudah memiliki onclick="closeNav()"
});