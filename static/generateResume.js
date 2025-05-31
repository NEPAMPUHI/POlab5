document.addEventListener('DOMContentLoaded', function () { 
    const generatePdfButton = document.getElementById('generate-pdf');
    if (generatePdfButton) { 
        generatePdfButton.addEventListener('click', function () {
            const name = document.getElementById('name')?.value || ''; 
            const experience = document.getElementById('experience')?.value || ''; 

            const { jsPDF } = window.jspdf;
            const doc = new jsPDF();

            doc.text(`Name: ${name}`, 10, 10);
            doc.text(`Work experience: ${experience}`, 10, 30);

            doc.save('resume.pdf');
        });
    } else {
        console.error(" нопка 'generate-pdf' не знайдена.");
    }
});