#include "arcpch.h"

#ifdef ARC_PLATFORM_WINDOWS

#include "Arc/Utils/PlatformUtils.h"

#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <ShlObj_core.h>
#include <comdef.h>
#include <commdlg.h>
#include <shellapi.h>

#include "Arc/Core/Application.h"

namespace ArcEngine
{
	std::string FileDialogs::OpenFolder()
	{
		WCHAR szTitle[MAX_PATH];
		BROWSEINFO bi;
		bi.hwndOwner = glfwGetWin32Window(static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow()));
		bi.pidlRoot = nullptr;
		bi.pszDisplayName = szTitle;
		bi.lpszTitle = L"Select a folder containing the response file";
		bi.ulFlags = BIF_RETURNONLYFSDIRS;
		bi.lpfn = nullptr;
		bi.lParam = 0;
		if (const auto* pidl = SHBrowseForFolder(&bi))
		{
			if (SHGetPathFromIDList(pidl, szTitle))
			{
				const _bstr_t b(szTitle);
				const char* c = b;
				return c;
			}
		}
		return "";
	}

	std::string FileDialogs::OpenFile(const char* filter)
	{
		OPENFILENAMEA ofn;
		CHAR szFile[260] = { 0 };
		ZeroMemory(&ofn, sizeof(OPENFILENAME));
		ofn.lStructSize = sizeof(OPENFILENAME);
		ofn.hwndOwner = glfwGetWin32Window(static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow()));
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = sizeof(szFile);
		ofn.lpstrFilter = filter;
		ofn.nFilterIndex = 1;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
		if (GetOpenFileNameA(&ofn) == TRUE)
		{
			return ofn.lpstrFile;
		}
		return "";
	}

	std::string FileDialogs::SaveFile(const char* filter)
	{
		OPENFILENAMEA ofn;
		CHAR szFile[260] = { 0 };
		ZeroMemory(&ofn, sizeof(OPENFILENAME));
		ofn.lStructSize = sizeof(OPENFILENAME);
		ofn.hwndOwner = glfwGetWin32Window(static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow()));
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = sizeof(szFile);
		ofn.lpstrFilter = filter;
		ofn.nFilterIndex = 1;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
		if (GetSaveFileNameA(&ofn) == TRUE)
		{
			return ofn.lpstrFile;
		}
		return "";
	}

	void FileDialogs::OpenFolderAndSelectItem(const char* path)
	{
		const _bstr_t widePath(path);
		if (const LPITEMIDLIST pidl = ILCreateFromPath(widePath))
		{
			SHOpenFolderAndSelectItems(pidl, 0, nullptr, 0);
			ILFree(pidl);
		}
	}

	void FileDialogs::OpenFileWithProgram(const char* path)
	{
		const _bstr_t widePath(path);
		ShellExecute(nullptr, L"open", widePath, nullptr, nullptr, SW_RESTORE);
	}
}

#endif
