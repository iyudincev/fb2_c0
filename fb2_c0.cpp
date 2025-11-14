#include <stdint.h>
#include <vector>

#include "../common/plugin.h"
#include "mapping.hpp"
#include "xml_tools.hpp"
#include "mz.h"
#include "mz_zip.h"
#include "mz_strm.h"
#include "mz_strm_buf.h"
#include "mz_strm_mem.h"
#include "mz_strm_os.h"


const int MAX_ARCHIVE_SIZE = 200000000;
const int BYTES_READ = 0x1000;

enum ColumnType {
	CT_UNKNOWN,
	CT_TITLE,
	CT_AUTHOR_FIRST_NAME,
	CT_AUTHOR_LAST_NAME,
	CT_GENRE,
	CT_DATE,
	CT_SEQUENCE,
	CT_NUMBER,
};

// {2E5FB71C-E49F-4C3D-AC14-83A8C223B115}
static GUID PluginGUID =
{ 0x2E5FB71C, 0xE49F, 0x4C3D, { 0xAC, 0x14, 0x83, 0xA8, 0xC2, 0x23, 0xB1, 0x15 } };


/* FAR */
PluginStartupInfo psi;
FarStandardFunctions fsf;

BOOL WINAPI DllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
	return TRUE;
}

int WINAPI GetMinFarVersionW() {
	return MAKEFARVERSION2(FARMANAGERVERSION_MAJOR, FARMANAGERVERSION_MINOR, FARMANAGERVERSION_BUILD);
}

void WINAPI SetStartupInfoW(const struct PluginStartupInfo *Info) {
	::psi = *Info;
	::fsf = *(Info->FSF);
}

void WINAPI GetPluginInfoW(struct PluginInfo *Info) {
	Info->StructSize = sizeof(struct PluginInfo);
	Info->Flags = PF_PRELOAD;
}

void WINAPI GetGlobalInfoW(struct GlobalInfo *Info) {
	Info->MinFarVersion = MAKEFARVERSION(3, 0, 0, 4040, VS_RELEASE);
	Info->Version = MAKEFARVERSION(1, 0, 0, 0, VS_RELEASE);
	Info->Guid = PluginGUID;
	Info->Title = L"FB2_C0";
	Info->Description = L"FictionBook metadata in C0 columns";
	Info->Author = L"Igor.Yudincev@gmail.com";
}

ColumnType GetColumnByName(const wchar_t *name) {
	if (fsf.LStricmp(name, L"title") == 0) return CT_TITLE;
	if (fsf.LStricmp(name, L"first-name") == 0) return CT_AUTHOR_FIRST_NAME;
	if (fsf.LStricmp(name, L"last-name") == 0) return CT_AUTHOR_LAST_NAME;
	if (fsf.LStricmp(name, L"genre") == 0) return CT_GENRE;
	if (fsf.LStricmp(name, L"date") == 0) return CT_DATE;
	if (fsf.LStricmp(name, L"sequence") == 0) return CT_SEQUENCE;
	if (fsf.LStricmp(name, L"number") == 0) return CT_NUMBER;
	return CT_UNKNOWN;
}

bool EndsWith(const char *name, const char *tail) {
	size_t nNameChars = strlen(name);
	size_t nTailChars = strlen(tail);
	if (nNameChars < nTailChars) {
		return false;
	}
	return lstrcmpiA(name + nNameChars - nTailChars, tail) == 0;
}

bool EndsWith(const wchar_t *name, const wchar_t *tail) {
	size_t nNameChars = wcslen(name);
	size_t nTailChars = wcslen(tail);
	if (nNameChars < nTailChars) {
		return false;
	}
	return fsf.LStricmp(name + nNameChars - nTailChars, tail) == 0;
}

// Function to extract a specific file from a zip buffer into an output memory buffer
bool extract_file_from_zip_buffer(const void *zip_buffer, int32_t zip_buffer_size,
								  const char *ext, int32_t extract_bytes,
								  std::vector<uint8_t>& output_buffer) {
	void *mem_stream = nullptr;
	void *zip_handle = mz_zip_create();
	int32_t err = MZ_OK;
	bool success = false;

	// Create a memory stream and set its buffer to the input zip data
	mem_stream = mz_stream_mem_create();
	mz_stream_mem_set_buffer(mem_stream, (void *)zip_buffer, (int32_t)zip_buffer_size);
	err = mz_stream_open(mem_stream, nullptr, MZ_OPEN_MODE_READ);
	if (err != MZ_OK) {
		mz_stream_mem_delete(&mem_stream);
		mz_zip_delete(&zip_handle);
		return false;
	}

	// Open the zip archive from the memory stream
	err = mz_zip_open(zip_handle, mem_stream, MZ_OPEN_MODE_READ);
	if (err != MZ_OK) {
		mz_stream_close(mem_stream);
		mz_stream_mem_delete(&mem_stream);
		mz_zip_delete(&zip_handle);
		return false;
	}

	// Locate the first file with the given extension within the archive
	bool found = false;
	err = mz_zip_goto_first_entry(zip_handle);
	while (err == MZ_OK) {
		if (mz_zip_entry_is_dir(zip_handle) != MZ_OK &&
			mz_zip_entry_is_symlink(zip_handle) != MZ_OK)
		{
			found = true;
			break;
		}
		err = mz_zip_goto_next_entry(zip_handle);
	}

	if (found) {
		found = false;
		err = mz_zip_entry_read_open(zip_handle, 0, nullptr);
		if (err == MZ_OK) {
			mz_zip_file *file_info;
			err = mz_zip_entry_get_info(zip_handle, &file_info);
			if (err == MZ_OK) {
				if (EndsWith(file_info->filename, ext)) {
					found = true;
				}
			}
			mz_zip_entry_close(zip_handle);
		}
	}

	if (found) {
		// Open the file entry
		err = mz_zip_entry_read_open(zip_handle, 0, nullptr);
		if (err == MZ_OK) {
			// Read the file data into the output buffer
			mz_zip_file *file_info;
			err = mz_zip_entry_get_info(zip_handle, &file_info);
			if (err == MZ_OK) {
				int64_t buffer_length =
					(extract_bytes < 0) ? file_info->uncompressed_size :
					(extract_bytes < file_info->uncompressed_size) ? extract_bytes :
					file_info->uncompressed_size;
				if (buffer_length > 0) {
					output_buffer.resize((size_t)buffer_length);
					int32_t read_bytes = mz_zip_entry_read(zip_handle, output_buffer.data(), (int32_t)output_buffer.size());
					if (read_bytes == buffer_length) {
						success = true;
					} else {
						output_buffer.clear();
					}
				}
			}

			// Close the file entry
			mz_zip_entry_close(zip_handle);
		}
	}

	// Close the zip archive and memory stream
	mz_zip_close(zip_handle);
	mz_stream_close(mem_stream);
	mz_stream_mem_delete(&mem_stream);
	mz_zip_delete(&zip_handle);

	return success;
}

const wchar_t *CreateValue(const std::string &text, int encoding) {
	if (text.empty()) return nullptr;
	int nWideChars = MultiByteToWideChar(encoding, 0, text.data(), (int)text.size(), nullptr, 0);
	if (nWideChars == 0) return nullptr;
	wchar_t *result = new wchar_t[nWideChars + 1];
	MultiByteToWideChar(encoding, 0, text.data(), (int)text.size(), result, nWideChars);
	result[nWideChars] = L'\0';
	return result;
}

intptr_t WINAPI GetContentFieldsW(const struct GetContentFieldsInfo *Info) {
	for (size_t i = 0; i < Info->Count; i++) {
		if (GetColumnByName(Info->Names[i]) != CT_UNKNOWN)
			return TRUE;
	}
	return FALSE;
}

intptr_t WINAPI GetContentDataW(struct GetContentDataInfo *Info) {
	if (!Info || !Info->FilePath)
		return FALSE;

	const wchar_t *pszFileName = wcsrchr(Info->FilePath, L'\\');
	if (!pszFileName) return FALSE;
	pszFileName++;

	const char *pContent = nullptr;
	size_t nContent;
	KFileMapping m;
	std::vector<uint8_t> Unpacked;

	if (EndsWith(pszFileName, L".fb2")) {
		if (m.open(Info->FilePath)) {
			pContent = m.value();
			nContent = m.getSize();
		}
	}
	else if (EndsWith(pszFileName, L".zip")) {
		if (m.open(Info->FilePath) &&
			m.getSize() <= MAX_ARCHIVE_SIZE &&
			extract_file_from_zip_buffer(m.value(), m.getSize(), ".fb2", BYTES_READ, Unpacked))
		{
			pContent = (const char *)Unpacked.data();
			nContent = Unpacked.size();
		}
	}

	if (!pContent) {
		return FALSE;
	}

	int encoding = CP_UTF8;
	if (GetStringAttribute(pContent, nContent, "?xml", "encoding") == "windows-1251") {
		encoding = CP_ACP;
	}

	const char *pInfoBegin, *pInfoEnd;
	if (!FindElement(pContent, nContent, "title-info", &pInfoBegin, &pInfoEnd)) {
		return FALSE;
	}

	intptr_t result = FALSE;
	int nInfo = (int)(pInfoEnd - pInfoBegin);
	for (size_t i = 0; i < Info->Count; i++) {
		std::string value;
		switch (GetColumnByName(Info->Names[i])) {
		case CT_TITLE:
			value = GetElementText(pInfoBegin, nInfo, "book-title");
			if (!value.empty()) {
				Info->Values[i] = CreateValue(value, encoding);
				result = TRUE;
			}
			break;
		case CT_AUTHOR_FIRST_NAME:
			value = GetElementText(pInfoBegin, nInfo, "first-name");
			if (!value.empty()) {
				Info->Values[i] = CreateValue(value, encoding);
				result = TRUE;
			}
			break;
		case CT_AUTHOR_LAST_NAME:
			value = GetElementText(pInfoBegin, nInfo, "last-name");
			if (!value.empty()) {
				Info->Values[i] = CreateValue(value, encoding);
				result = TRUE;
			}
			break;
		case CT_GENRE:
			value = GetElementText(pInfoBegin, nInfo, "genre");
			if (!value.empty()) {
				Info->Values[i] = CreateValue(value, encoding);
				result = TRUE;
			}
			break;
		case CT_DATE:
			value = GetStringAttribute(pInfoBegin, nInfo, "date", "value");
			if (!value.empty()) {
				Info->Values[i] = CreateValue(value, encoding);
				result = TRUE;
			}
			break;
		case CT_SEQUENCE:
			value = GetStringAttribute(pInfoBegin, nInfo, "sequence", "name");
			if (!value.empty()) {
				Info->Values[i] = CreateValue(value, encoding);
				result = TRUE;
			}
			break;
		case CT_NUMBER:
			value = GetStringAttribute(pInfoBegin, nInfo, "sequence", "number");
			if (!value.empty()) {
				Info->Values[i] = CreateValue(value, encoding);
				result = TRUE;
			}
			break;
		}
	}

	return result;
}

void WINAPI FreeContentDataW(const struct GetContentDataInfo *Info) {
	for (size_t i = 0; i < Info->Count; i++) {
		if (GetColumnByName(Info->Names[i]) != CT_UNKNOWN) {
			const wchar_t* value = Info->Values[i];
			delete[] value;
		}
	}
}
