const urlInput = document.getElementById("url-input");
const sendButton = document.getElementById("send-btn");
const methodSelect = document.getElementById("method-select");
const headersInput = document.getElementById("headers-input");
const bodyInput = document.getElementById("body-input");
const responseStatus = document.getElementById("response-status");
const responseHeaders = document.getElementById("response-headers");
const responseBody = document.getElementById("response-body");
const themeSelect = document.getElementById("theme-select");
const presetSelect = document.getElementById("preset-select");
const loadPresetButton = document.getElementById("load-preset-btn");
const REQUEST_TIMEOUT_MS = 10000;
const THEME_COOKIE_NAME = "theme";

const REQUEST_PRESETS = {
	"get-home": {
		method: "GET",
		path: "/",
		headers: "Accept: text/html",
		body: ""
	},
	"post-demo": {
		method: "POST",
		path: "/files/demo.txt",
		headers: "Content-Type: text/plain",
		body: "Hello from the HTTP tester.\n"
	},
	"get-demo": {
		method: "GET",
		path: "/files/demo.txt",
		headers: "Accept: text/plain",
		body: ""
	},
	"delete-demo": {
		method: "DELETE",
		path: "/files/demo.txt",
		headers: "",
		body: ""
	}
};

const THEMES = {
	terminal: {
		background: "#000000",
		text: "#00ff00",
		accent: "#00ff00"
	},
	light: {
		background: "#ffffff",
		text: "#111111",
		accent: "#2563eb"
	},
	dark: {
		background: "#111111",
		text: "#f5f5f5",
		accent: "#60a5fa"
	},
	semantic: {
		background: "#0f172a",
		text: "#e2e8f0",
		accent: "#38bdf8",
		status2xx: "#22c55e",
		status3xx: "#f59e0b",
		status4xx: "#f97316",
		status5xx: "#ef4444"
	}
};

let activeThemeName = "terminal";

function getSafeThemeName(themeName) {
	if (THEMES[themeName]) {
		return themeName;
	}

	return "terminal";
}

function setThemeCookie(themeName) {
	document.cookie = THEME_COOKIE_NAME + "=" + encodeURIComponent(themeName) + "; path=/";
}

function getThemeCookie() {
	const cookieParts = document.cookie.split(";");
	for (let i = 0; i < cookieParts.length; i += 1) {
		const part = cookieParts[i].trim();
		if (part.indexOf(THEME_COOKIE_NAME + "=") === 0) {
			return decodeURIComponent(part.slice(THEME_COOKIE_NAME.length + 1));
		}
	}

	return "";
}

function clearStatusClasses() {
	responseStatus.classList.remove("status-2xx", "status-3xx", "status-4xx", "status-5xx");
}

function updateStatusClass(statusCode) {
	clearStatusClasses();

	if (statusCode >= 200 && statusCode < 300) {
		responseStatus.classList.add("status-2xx");
		return;
	}

	if (statusCode >= 300 && statusCode < 400) {
		responseStatus.classList.add("status-3xx");
		return;
	}

	if (statusCode >= 400 && statusCode < 500) {
		responseStatus.classList.add("status-4xx");
		return;
	}

	if (statusCode >= 500 && statusCode < 600) {
		responseStatus.classList.add("status-5xx");
	}
}

function applySemanticStatusColor() {
	responseStatus.style.color = "";

	if (activeThemeName !== "semantic") {
		return;
	}

	const semanticTheme = THEMES.semantic;

	if (responseStatus.classList.contains("status-2xx")) {
		responseStatus.style.color = semanticTheme.status2xx;
		return;
	}

	if (responseStatus.classList.contains("status-3xx")) {
		responseStatus.style.color = semanticTheme.status3xx;
		return;
	}

	if (responseStatus.classList.contains("status-4xx")) {
		responseStatus.style.color = semanticTheme.status4xx;
		return;
	}

	if (responseStatus.classList.contains("status-5xx")) {
		responseStatus.style.color = semanticTheme.status5xx;
	}
}

function applyTheme(themeName) {
	const safeThemeName = getSafeThemeName(themeName);
	const theme = THEMES[safeThemeName];
	activeThemeName = safeThemeName;

	document.body.style.backgroundColor = theme.background;
	document.body.style.color = theme.text;

	const controls = [
		themeSelect,
		methodSelect,
		urlInput,
		headersInput,
		bodyInput,
		presetSelect,
		loadPresetButton,
		sendButton
	];
	for (let i = 0; i < controls.length; i += 1) {
		controls[i].style.backgroundColor = theme.background;
		controls[i].style.color = theme.text;
		controls[i].style.borderColor = theme.accent;
	}

	responseHeaders.style.border = "1px solid " + theme.accent;
	responseBody.style.border = "1px solid " + theme.accent;

	applySemanticStatusColor();
}

function getBaseUrl() {
	if (window.location.origin && window.location.origin !== "null") {
		return window.location.origin;
	}

	return "http://127.0.0.1:8080";
}

function loadPreset(presetName) {
	const preset = REQUEST_PRESETS[presetName];
	if (!preset) {
		return;
	}

	methodSelect.value = preset.method;
	urlInput.value = getBaseUrl() + preset.path;
	headersInput.value = preset.headers;
	bodyInput.value = preset.body;
}

themeSelect.addEventListener("change", () => {
	const selectedTheme = getSafeThemeName(themeSelect.value);
	applyTheme(selectedTheme);
	setThemeCookie(selectedTheme);
});

const cookieTheme = getThemeCookie();
const initialTheme = getSafeThemeName(cookieTheme || "terminal");
themeSelect.value = initialTheme;
applyTheme(initialTheme);

loadPresetButton.addEventListener("click", () => {
	loadPreset(presetSelect.value);
});

sendButton.addEventListener("click", async () => {
	const method = methodSelect.value;
	const url = urlInput.value;
	const rawHeaders = headersInput.value;
	const body = bodyInput.value;

	responseStatus.textContent = "";
	responseHeaders.textContent = "";
	responseBody.textContent = "";
	clearStatusClasses();
	applySemanticStatusColor();
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
		updateStatusClass(response.status);
		applySemanticStatusColor();

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
			clearStatusClasses();
			applySemanticStatusColor();
			return;
		}

		responseStatus.textContent = "Error";
		responseBody.textContent = error.message;
		clearStatusClasses();
		applySemanticStatusColor();
	}
});
