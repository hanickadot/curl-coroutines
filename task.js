async function download() {
	const a = fetch("http://hanicka.net");
	const b = fetch("http://google.com");
	
	return (await a).text() + (await b).text();
}