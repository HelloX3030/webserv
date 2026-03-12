## uploads.conf

file upload scenario. client POSTs a file; server saves it to disk.

demonstrates: upload_enable, upload_store, POST in allowed_methods,
client_max_body_size set high enough to accept uploads.

path dependencies: www/uploads/ (server writes uploaded files here)