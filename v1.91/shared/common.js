hljs.configure(
	{
		tabReplace: '    ',
	}
);
hljs.highlightAll();

hljsAsar.init();

function toggle_visibility(id)
{
	var e =document.getElementById(id);
	var label = document.getElementById(id.concat("-toggle"));
	
	if(e.style.display == 'none')
	{
		label.innerHTML = label.innerHTML.replace("[+] Expand","[-] Collapse");
		e.style.display = 'block';
	}
	else
	{
		label.innerHTML = label.innerHTML.replace("[-] Collapse","[+] Expand");
		e.style.display = 'none';
	
	}
}
