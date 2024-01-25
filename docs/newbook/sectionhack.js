(function() {
    let thispage = document.querySelector("#sidebar .active");
    if(thispage) {
        let the_list = document.createElement("ol");
        the_list.classList.add("section");
        for(let section of document.querySelectorAll("h2 .header")) {
            let list_el = document.createElement("li");
            list_el.className = "chapter-item expanded";
            list_el.append(section.cloneNode(true));
            the_list.append(list_el);
        }
        let list_wrap = document.createElement("li");
        list_wrap.append(the_list);
        thispage.parentElement.insertAdjacentElement("afterend", list_wrap);
    }
})();
