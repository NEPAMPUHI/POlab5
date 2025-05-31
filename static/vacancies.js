fetch('vacancies.json') 
    .then(response => response.json())
    .then(data => {
        const vacanciesContainer = document.getElementById('vacancies-container');
        let vacanciesHTML = '';
        data.forEach(vacancy => {
            vacanciesHTML += `
        <div class="vacancy">
          <h3>${vacancy.title}</h3>
          <p>${vacancy.company}, ${vacancy.location}</p>
        </div>
      `;
        });
        vacanciesContainer.innerHTML = vacanciesHTML;
    });
