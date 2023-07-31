#include "ShaderCompilation.h"
#include "..\packages\DirectXShaderCompiler\inc\d3d12shader.h" // must before dxcapi.h
#include "..\packages\DirectXShaderCompiler\inc\dxcapi.h"

#include "Graphics\Direct3D12\D3D12Core.h"
#include "Graphics\Direct3D12\D3D12Shaders.h"

#include <fstream>
#include <filesystem>
// NOTE: we wouldn't need to do this if DXC had a Nuget package.
#pragma comment(lib, "../packages/DirectXShaderCompiler/lib/x64/dxcompiler.lib")
using namespace ferraris;
using namespace ferraris::graphics::d3d12::shaders;
using namespace Microsoft::WRL;
namespace {

struct shader_file_info
{
	const char*			file;
	const char*			function;
	engine_shader::id	id;
	shader_type::type	type;
};

constexpr shader_file_info shader_files[]
{
	{"FullScreenTriangle.hlsl", "FullScreenTriangleVS", engine_shader::fullscreen_triangle_vs, shader_type::vertex},
	{"FillColor.hlsl", "FillColorPS", engine_shader::fill_color_ps, shader_type::pixel},
	{"PostProcess.hlsl", "PostProcessPS", engine_shader::post_process_ps, shader_type::pixel},
};
static_assert(_countof(shader_files) == engine_shader::count);

constexpr const char* shaders_source_path{ "../../Engine/Graphics/Direct3D12/Shaders/" };

std::wstring
to_wstring(const char* c)
{
	std::string s{ c };
	return { s.begin(), s.end() };
}

class shader_compiler
{
public:
	shader_compiler()
	{
		HRESULT hr{ S_OK };
		DXCall(hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&_compiler)));
		if (FAILED(hr)) return;
		DXCall(hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&_utils)));
		if (FAILED(hr)) return;
		DXCall(hr = _utils->CreateDefaultIncludeHandler(&_include_handler));
		if (FAILED(hr)) return;

	}
	DISABLE_COPY_AND_MOVE(shader_compiler);

	// Loading file + compile
	IDxcBlob* compile(shader_file_info info, std::filesystem::path full_path)
	{
		assert(_compiler && _utils && _include_handler);
		HRESULT hr{ S_OK };
		// Load the source file using util interface.
		ComPtr<IDxcBlobEncoding> source_blob{};
		DXCall(hr = _utils->LoadFile(full_path.c_str(), nullptr, &source_blob));
		if (FAILED(hr)) return nullptr;
		assert(source_blob && source_blob->GetBufferSize());

		// Convert the info to wstring for serialization.
		std::wstring file{ to_wstring(info.file) };
		std::wstring func{ to_wstring(info.function) };
		std::wstring prof{ to_wstring(_profile_strings[(u32)info.type]) };
		std::wstring inc{ to_wstring(shaders_source_path) };

		// Usage of the DxcCompiler
		// https://github.com/microsoft/DirectXShaderCompiler/wiki/Using-dxc.exe-and-dxcompiler.dll
		LPCWSTR args[]
		{
			file.c_str(),							// Optional shader source file name for error reporting
			L"-E",
			func.c_str(),							// Entry function
			L"-T",
			prof.c_str(),							// Target profile
			L"-I",
			inc.c_str(),							// Include path
			DXC_ARG_ALL_RESOURCES_BOUND,
#if _DEBUG
			DXC_ARG_DEBUG,
			DXC_ARG_SKIP_OPTIMIZATIONS,
#else
			DXC_ARG_OPTIMIZATION_LEVEL3,
#endif
			DXC_ARG_WARNINGS_ARE_ERRORS,
			L"-Qstrip_reflect",						// Strip reflections into a separate blob
			L"-Qstrip_debug"						// Strip debug information into a separate blob
		};

		OutputDebugStringA("Compiling");
		OutputDebugStringA(info.file);

		return compile(source_blob.Get(), args, _countof(args));
	}
	// compile only
	IDxcBlob* compile(IDxcBlobEncoding* source_blob, LPCWSTR* args, u32 args_num)
	{
		DxcBuffer buffer{};
		buffer.Encoding = DXC_CP_ACP;				// use if blob contain text. For binary or ANSI code DXC_CP_ACP
		buffer.Ptr = source_blob->GetBufferPointer();
		buffer.Size = source_blob->GetBufferSize();

		HRESULT hr{ S_OK };
		ComPtr<IDxcResult> results{ nullptr };
		DXCall(hr = _compiler->Compile(&buffer, args, args_num, _include_handler.Get(), IID_PPV_ARGS(&results)));
		if (FAILED(hr)) return nullptr;

		ComPtr<IDxcBlobUtf8> errors{ nullptr };
		DXCall(hr = results->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr));
		if (FAILED(hr)) return nullptr;

		if (errors && errors->GetStringLength())
		{
			OutputDebugStringA("\nShader compilation error: \n");
			OutputDebugStringA(errors->GetStringPointer());
		}
		else
		{
			OutputDebugStringA(" [ Succeeded ]");
		}
		OutputDebugStringA("\n");

		HRESULT status{ S_OK };
		DXCall(hr = results->GetStatus(&status));
		if (FAILED(hr) || FAILED(status)) return nullptr;

		ComPtr<IDxcBlob> shader{ nullptr };
		DXCall(hr = results->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shader), nullptr));
		if (FAILED(hr)) return nullptr;

		return shader.Detach();
	}
private:

	// NOTE: Shader Model 6.x can also be used (AS and MS are only supported from SM6.5 on).
	constexpr static const char* _profile_strings[]{ "vs_6_5", "hs_6_5", "ds_6_5", "gs_6_5", "ps_6_5", "cs_6_5", "as_6_5", "ms_6_5" };
	static_assert(_countof(_profile_strings) == shader_type::count);

	// NOTE: Instead of using the Compiler from local SDK. We pull the lastest SDK of Shader compiler from github.
	ComPtr<IDxcCompiler3>			_compiler{};
	ComPtr<IDxcUtils>				_utils{};
	ComPtr<IDxcIncludeHandler>		_include_handler{};

};


decltype(auto)
get_engine_shaders_path()
{
	return std::filesystem::path{ graphics::get_engine_shaders_path(graphics::graphics_platform::direct3d12) };
}

bool
compiled_shaders_are_up_to_date()
{
	auto engine_shaders_path = get_engine_shaders_path();
	if (!std::filesystem::exists(engine_shaders_path))return false;
	auto shaders_compilation_time = std::filesystem::last_write_time(engine_shaders_path);

	std::filesystem::path path{};
	std::filesystem::path full_path{};

	// Check if either of engine shader source files is newer than the compiled shader file.
	// In that case, we need to recompile.
	for (u32 i{ 0 }; i < engine_shader::count; ++i)
	{
		auto& info = shader_files[i];

		path = shaders_source_path;
		path += info.file;
		full_path = path;
		if (!std::filesystem::exists(full_path))return false;

		auto shader_file_time = std::filesystem::last_write_time(full_path);
		if (shader_file_time > shaders_compilation_time)
		{
			return false;
		}
	}
	return true;
}

bool
save_compiled_shaders(const utl::vector<ComPtr<IDxcBlob>>& shaders)
{
	auto engine_shaders_path = get_engine_shaders_path();
	std::filesystem::create_directories(engine_shaders_path.parent_path());
	std::ofstream file{ engine_shaders_path, std::ios::out | std::ios::binary };
	if (!file || !std::filesystem::exists(engine_shaders_path))
	{
		file.close();
		return false;
	}

	for (auto& shader : shaders)
	{
		const D3D12_SHADER_BYTECODE byte_code{ shader->GetBufferPointer(), shader->GetBufferSize() };
		file.write((char*)&byte_code.BytecodeLength, sizeof(byte_code.BytecodeLength));
		file.write((char*)byte_code.pShaderBytecode, byte_code.BytecodeLength);
	}

	file.close();
	return true;
}
} // anonymous namespace


bool
compile_shaders()
{
	if (compiled_shaders_are_up_to_date()) return true;
	utl::vector<ComPtr<IDxcBlob>> shaders;
	std::filesystem::path path{};
	std::filesystem::path full_path{};
	shader_compiler compiler{};
	// compiled shaders in a buffer in the same order of compilation.
	for (u32 i{ 0 }; i < engine_shader::count; ++i)
	{
		auto& info = shader_files[i];

		path = shaders_source_path;
		path += info.file;
		full_path = path;
		if (!std::filesystem::exists(full_path)) return false;
		ComPtr<IDxcBlob> compiled_shader{ compiler.compile(info, full_path) };
		if (compiled_shader != nullptr && compiled_shader->GetBufferPointer() && compiled_shader->GetBufferSize())
		{
			shaders.emplace_back(std::move(compiled_shader));
		}
		else
		{
			return false;
		}
	}
	return save_compiled_shaders(shaders);
}