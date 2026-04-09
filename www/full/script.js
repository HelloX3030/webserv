const urlInput = document.getElementById("url-input");
const sendButton = document.getElementById("send-btn");
const methodSelect = document.getElementById("method-select");
const headersInput = document.getElementById("headers-input");
const bodyInput = document.getElementById("body-input");
const responseStatus = document.getElementById("response-status");
const responseHeaders = document.getElementById("response-headers");
const responseBody = document.getElementById("response-body");
const REQUEST_TIMEOUT_MS = 10000;

sendButton.addEventListener("click", async () => {
	const method = methodSelect.value;
	const url = urlInput.value;
	const rawHeaders = headersInput.value;
	const body = bodyInput.value;

	responseStatus.textContent = "";
	responseHeaders.textContent = "";
	responseBody.textContent = "";
	let timeoutId;

	try {
		const headers = {};
		const lines = rawHeaders.split("\n");

		for (let i = 0; i < lines.length; i += 1) {
			const line = lines[i].trim();
			if (!line) {
				continue;
			}

			const separatorIndex = line.indexOf(":");
			if (separatorIndex === -1) {
				throw new Error("Invalid header line: " + line);
			}

			const key = line.slice(0, separatorIndex).trim();
			const value = line.slice(separatorIndex + 1).trim();

			if (!key) {
				throw new Error("Invalid header name in line: " + line);
			}

			headers[key] = value;
		}

		const controller = new AbortController();
		timeoutId = setTimeout(() => {
			controller.abort();
		}, REQUEST_TIMEOUT_MS);

		const options = {
			method: method,
			headers: headers,
			signal: controller.signal
		};

		if (method !== "GET") {
			options.body = body;
		}

		const response = await fetch(url, options);
		clearTimeout(timeoutId);

		responseStatus.textContent = response.status + " " + response.statusText;

		let rawResponseHeaders = "";
		response.headers.forEach((value, key) => {
			rawResponseHeaders += key + ": " + value + "\n";
		});
		responseHeaders.textContent = rawResponseHeaders;

		const responseText = await response.text();
		responseBody.textContent = responseText;
	} catch (error) {
		if (timeoutId) {
			clearTimeout(timeoutId);
		}

		if (error.name === "AbortError") {
			responseStatus.textContent = "Timeout";
			responseBody.textContent = "Request timed out after " + REQUEST_TIMEOUT_MS + " ms";
			return;
		}

		responseStatus.textContent = "Error";
		responseBody.textContent = error.message;
	}
});
