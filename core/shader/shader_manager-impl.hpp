
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace render{

///////////////////////////////////////////////////////////////////////////////////////////////////
//shader_manager
///////////////////////////////////////////////////////////////////////////////////////////////////

//コンストラクタ
inline shader_manager::shader_manager(iface::render_device& device) : m_terminate(), m_watcher_impl(shader_src_dir)
{
	m_worker = std::thread([&]()
	{
		while(not(m_terminate))
		{
			wstring filename;
			{
				std::unique_lock<std::mutex> lock(m_queue_mtx);
				if(m_queue.empty())
				{
					m_cv.wait(lock, [&](){ return not(m_queue.empty()) || m_terminate; });
					if(m_terminate){ break; }
				}

				filename = format(m_queue.front());
				m_queue.pop();
			}

			if(filename.find(L".sdf.json") != wstring::npos)
			{
				std::cout << utf16_to_utf8(filename) << (char*)u8"をコンパイルします" << std::endl;

				auto& record = *get(filename);

				shader_file_ptr p_file;
				unordered_set<wstring> dependent;
				std::filesystem::file_time_type last_write_time;
				try
				{
					auto get_last_write_time = [&](const wstring& filename)
					{
						if(not(std::filesystem::exists(filename)))
							throw std::runtime_error(utf16_to_utf8(filename + L"が存在しません"));
						return std::filesystem::last_write_time(filename);
					};

					//最新状態なら処理しない
					last_write_time = get_last_write_time(shader_src_dir + filename);
					for(auto& filename : record.dependent)
					{
						const auto write_time = get_last_write_time(shader_src_dir + filename);
						if(last_write_time < write_time)
							last_write_time = write_time;
					}
					if(record.last_write_time == last_write_time)
					{
						std::cout << utf16_to_utf8(filename) << (char*)u8"最新状態のためコンパイルをスキップしました" << std::endl;
						continue;
					}

					//コンパイル&依存ファイルの収集
					p_file = create(device, filename, dependent);
				}
				catch(const std::exception& e)
				{
					std::cout << utf16_to_utf8(filename) << (char*)u8"のコンパイルに失敗しました" << std::endl;
					std::cout << e.what() << std::endl;

					//ファイル更新で再コンパイルできるように退避
					m_failed_files.push_back(filename);
					continue;
				}

				if(p_file && p_file->is_valid())
				{
					//依存ファイルの設定を消す
					for(auto& dependent_filename : record.dependent)
						m_depended[dependent_filename].erase(filename);
			
					//依存ファイルの再設定
					record.dependent = std::move(dependent);
					for(auto& dependent_filename : record.dependent)
						m_depended[dependent_filename].emplace(filename);

					//更新
					for(auto p_old = record.p_file;;)
					{
						auto p_update = p_old->mp_update.load();
						if(p_update != nullptr)
							p_old = std::move(p_update);
						else
						{
							p_old->mp_update = std::move(p_file);
							break;
						}
					}

					//最終更新時間を設定
					//(最終更新時間を計算してからファイル更新された場合は正確な時間じゃないが再度コンパイルされるだけなので問題はない)
					record.last_write_time = last_write_time;
					
					std::cout << utf16_to_utf8(filename) << (char*)u8"のコンパイルに成功しました" << std::endl;
				}
			}
			//hlslなど
			else
			{
				if(filename.ends_with(L" modified"))
				{
					filename.resize(filename.size() - wcslen(L" modified"));
					std::cout << utf16_to_utf8(filename) << (char*)u8"が修正されました" << std::endl;

					//自分に依存しているファイルをキューへ追加
					std::unique_lock<std::mutex> lock(m_queue_mtx);
					for(auto& depended_filename : m_depended[filename])
						m_queue.push(depended_filename);
				}
				else if(filename.ends_with(L" added"))
				{
					filename.resize(filename.size() - wcslen(L" added"));
					if(m_depended.contains(filename))
						std::cout << utf16_to_utf8(filename) << (char*)u8"が修正されました" << std::endl; //↓の問題で仕方なく
					else
						std::cout << utf16_to_utf8(filename) << (char*)u8"が追加されました" << std::endl;
					
					//失敗したファイルをキューへ追加
					std::unique_lock<std::mutex> lock(m_queue_mtx);
					for(auto& filename : m_failed_files)
						m_queue.push(filename);
					m_failed_files.clear();
					
					//新規ファイルは依存されていないので不要だが,modifiedがtmpファイルに対して起きてrenameで元ファイルが更新される場合は元ファイルにmodified判定が出ないので仕方なく
					//自分に依存しているファイルをキューへ追加
					for(auto& depended_filename : m_depended[filename])
						m_queue.push(depended_filename);
				}
			}
		}
	});

	m_watcher = std::thread([&]()
	{
		vector<nn::util::filesystem_watcher::change_info> infos;
		while(true)
		{
			//変更があるまで待機
			m_watcher_impl.wait();

			//変更を取得
			using namespace std::chrono_literals;
			std::this_thread::sleep_for(100ms);
			m_watcher_impl.get(infos);

			//終了の強制起床の場合
			if(infos.empty())
				break;

			for(auto& info : infos)
			{
				if(info.path.ends_with(L".sdf.json"))
				{
					switch(info.action)
					{
					case nn::util::filesystem_watcher::added:
					case nn::util::filesystem_watcher::modified:
					case nn::util::filesystem_watcher::renamed_new_name:
					{
						std::shared_lock<std::shared_mutex> lock(m_records_mtx);
						if(m_records.contains(info.path))
						{
							std::unique_lock<std::mutex> lock(m_queue_mtx);
							m_queue.push(info.path);
							m_cv.notify_one();
						}
						break;
					}}
				}
				//hlslなど
				else
				{
					bool is_valid = false;
					for(uint i = 0; i < _countof(shader_file_ext); i++)
					{
						if(info.path.ends_with(shader_file_ext[i]))
						{
							is_valid = true;
							break;
						}
					}
					if(not(is_valid))
						continue;

					switch(info.action)
					{
					case nn::util::filesystem_watcher::modified:
					{
						std::unique_lock<std::mutex> lock(m_queue_mtx);
						m_queue.push(info.path + L" modified");
						m_cv.notify_one();
						break;
					}
					case nn::util::filesystem_watcher::added:
					case nn::util::filesystem_watcher::renamed_new_name:
					{
						std::unique_lock<std::mutex> lock(m_queue_mtx);
						m_queue.push(info.path + L" added");
						m_cv.notify_one();
						break;
					}
					case nn::util::filesystem_watcher::removed:
					case nn::util::filesystem_watcher::renamed_old_name:
						//ファイル削除はコンパイル成功につながらないので何もしない
						break;
					}
				}
			}
		}
	});
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//デストラクタ
inline shader_manager::~shader_manager()
{
	m_terminate = true;
	m_cv.notify_one();
	m_worker.join();
	m_watcher_impl.awake();
	m_watcher.join();
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//シェーダファイルを返す
inline shader_file_holder shader_manager::create(wstring filename)
{
	filename = format(filename);
	shader_file_holder ret(get(filename)->p_file);
	std::unique_lock<std::mutex> lock(m_queue_mtx);
	m_queue.push(filename);
	m_cv.notify_one();
	return ret;
}

inline shader_manager::record *shader_manager::get(const wstring& filename)
{
	{
		std::shared_lock<std::shared_mutex> lock(m_records_mtx);
		if(auto it = m_records.find(filename); it != m_records.end())
			return &it->second;
	}

	{
		std::unique_lock<std::shared_mutex> lock(m_records_mtx);
		if(auto it = m_records.find(filename); it != m_records.end())
			return &it->second;

		auto& record = m_records.try_emplace(filename).first->second;
		record.p_file = std::make_shared<shader_file>();
		return &record;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//シェーダファイルを作成
inline shared_ptr<shader_file> shader_manager::create(iface::render_device& device, const wstring& filename, unordered_set<wstring>& dependent)
{
	//
	//エラーが起きても継続できるようにしないとダメ
	//

	json_file file(shader_src_dir + filename);
	auto& values = file.values();

	unordered_map<string, blend_desc> blend_descs;
	unordered_map<string, rasterizer_desc> rasterizer_descs;
	unordered_map<string, input_layout_desc> input_layout_descs;
	unordered_map<string, depth_stencil_desc> depth_stencil_descs;

	for(const json_file::values_t& state : values["blend_state"])
	{
		if(auto it = state.find("name"); it != state.end())
		{
			auto& desc = blend_descs[string(it->second[0])];
			if(auto it = state.find("independent_blend"); it != state.end())
				desc.independent_blend = bool(it->second[0]);

			if(auto it = state.find("render_target"); it != state.end())
			{
				uint i = 0;
				for(const json_file::values_t& state : it->second)
				{
					if(auto it = state.find("enable"); it != state.end())
						desc.render_target[i].enable = bool(it->second[0]);

					auto get_op = [&](const string& op)
					{
						if(op == "add")
							return blend_op_add;
						if(op == "subtract")
							return blend_op_subtract;
						if(op == "rev_subtract")
							return blend_op_rev_subtract;
						if(op == "min")
							return blend_op_min;
						if(op == "max")
							return blend_op_max;
						throw std::invalid_argument(op + "は認識できません");
					};
					auto get_factor = [&](const string& factor)
					{
						if(factor == "zero")
							return blend_factor_zero;
						if(factor == "one")
							return blend_factor_one;
						if(factor == "src_color")
							return blend_factor_src_color;
						if(factor == "inv_src_color")
							return blend_factor_inv_src_color;
						if(factor == "src_alpha")
							return blend_factor_src_alpha;
						if(factor == "inv_src_alpha")
							return blend_factor_inv_src_alpha;
						if(factor == "dst_color")
							return blend_factor_dst_color;
						if(factor == "inv_dst_color")
							return blend_factor_inv_dst_color;
						if(factor == "dst_alpha")
							return blend_factor_dst_alpha;
						if(factor == "inv_dst_alpha")
							return blend_factor_inv_dst_alpha;
						throw std::invalid_argument(factor + "は認識できません");
					};

					if(auto it = state.find("op"); it != state.end())
						desc.render_target[i].op = get_op(it->second[0]);
					if(auto it = state.find("src"); it != state.end())
						desc.render_target[i].src = get_factor(it->second[0]);
					if(auto it = state.find("dst"); it != state.end())
						desc.render_target[i].dst = get_factor(it->second[0]);

					if(auto it = state.find("op_alpha"); it != state.end())
						desc.render_target[i].op_alpha = get_op(it->second[0]);
					if(auto it = state.find("src_alpha"); it != state.end())
						desc.render_target[i].src_alpha = get_factor(it->second[0]);
					if(auto it = state.find("dst_alpha"); it != state.end())
						desc.render_target[i].dst_alpha = get_factor(it->second[0]);

					if(auto it = state.find("write_mask"); it != state.end())
					{
						uint write_mask = render_target_write_mask_none;
						for(const string& mask_string : it->second)
						{
							if(mask_string == "all")
								write_mask |= render_target_write_mask_all;
							else if(mask_string == "red")
								write_mask |= render_target_write_mask_red;
							else if(mask_string == "green")
								write_mask |= render_target_write_mask_green;
							else if(mask_string == "blue")
								write_mask |= render_target_write_mask_blue;
							else
								throw std::invalid_argument(mask_string + "は認識できません");

						}
						desc.render_target[i].write_mask = render_target_write_mask(write_mask);
					}
					i++;
				}
			}
		}
	}

	for(const json_file::values_t& state : values["depth_stencil_state"])
	{
		if(auto it = state.find("name"); it != state.end())
		{
			auto get_func = [&](const string& func)
			{
				if(func == "never")
					return comparison_func_never;
				if(func == "less")
					return comparison_func_less;
				if(func == "equal")
					return comparison_func_equal;
				if(func == "not_equal")
					return comparison_func_not_equal;
				if(func == "less_equal")
					return comparison_func_less_equal;
				if(func == "greater")
					return comparison_func_greater;
				if(func == "greater_equal")
					return comparison_func_greater_equal;
				if(func == "always")
					return comparison_func_always;
				throw std::invalid_argument(func + "は認識できません");
			};
			auto get_mask = [&](const string& mask_string)
			{
				uint mask = 0;
				mask += (isalpha(mask_string[2]) ? (mask_string[2] - 'a' + 10) : (mask_string[2] - '0')) * 16;
				mask += (isalpha(mask_string[3]) ? (mask_string[3] - 'a' + 10) : (mask_string[3] - '0')) * 1;
				return mask;
			};
			auto get_op = [&](const string& op_string)
			{
				if(op_string == "keep")
					return stencil_op_keep;
				if(op_string == "zero")
					return stencil_op_zero;
				if(op_string == "invert")
					return stencil_op_invert;
				if(op_string == "replace")
					return stencil_op_replace;
				if(op_string == "inc_sat")
					return stencil_op_inc_sat;
				if(op_string == "dec_sat")
					return stencil_op_dec_sat;
				if(op_string == "inc")
					return stencil_op_inc;
				if(op_string == "dec")
					return stencil_op_dec;
				throw std::invalid_argument(op_string + "は認識できません");
			};
			auto get_op_desc = [&](const json_file::values_t& values)
			{
				stencil_op_desc desc;
				if(auto it = values.find("stencil_fail_op"); it != values.end())
					desc.stencil_fail_op = get_op(it->second[0]);
				if(auto it = values.find("depth_fail_op"); it != values.end())
					desc.depth_fail_op = get_op(it->second[0]);
				if(auto it = values.find("pass_op"); it != values.end())
					desc.pass_op = get_op(it->second[0]);
				if(auto it = values.find("func"); it != values.end())
					desc.func = get_func(it->second[0]);
				return desc;
			};

			auto& desc = depth_stencil_descs[string(it->second[0])];
			if(auto it = state.find("depth_enable"); it != state.end())
				desc.depth_enabale = bool(it->second[0]);
			if(auto it = state.find("depth_write"); it != state.end())
				desc.depth_write = bool(it->second[0]);
			if(auto it = state.find("depth_func"); it != state.end())
				desc.depth_func = get_func(it->second[0]);

			if(auto it = state.find("stencil_enable"); it != state.end())
				desc.stencil_enable = bool(it->second[0]);
			if(auto it = state.find("stencil_read_mask"); it != state.end())
				desc.stencil_read_mask = get_mask(it->second[0]);
			if(auto it = state.find("stencil_write_mask"); it != state.end())
				desc.stencil_write_mask = get_mask(it->second[0]);

			if(auto it = state.find("front"); it != state.end())
				desc.front = get_op_desc(it->second[0]);
			if(auto it = state.find("back"); it != state.end())
				desc.back = get_op_desc(it->second[0]);
		}
	}
	
	for(const json_file::values_t& state : values["rasterizer_state"])
	{
		if(auto it = state.find("name"); it != state.end())
		{
			auto& desc = rasterizer_descs[string(it->second[0])];
			if(auto it = state.find("fill"); it != state.end())
				desc.fill = bool(it->second[0]);
			if(auto it = state.find("depth_bias"); it != state.end())
				desc.depth_bias = int(it->second[0]);
			if(auto it = state.find("depth_bias_clamp"); it != state.end())
				desc.depth_bias_clamp = float(it->second[0]);
			if(auto it = state.find("slope_scaled_depth_bias"); it != state.end())
				desc.slope_scaled_depth_bias = float(it->second[0]);
			if(auto it = state.find("conservative"); it != state.end())
				desc.conservative = bool(it->second[0]);

			if(auto it = state.find("cull"); it != state.end())
			{
				const string& cull_mode = it->second[0];
				if(cull_mode == "none")
					desc.cull_mode = cull_mode_none;
				else if(cull_mode == "front")
					desc.cull_mode = cull_mode_front;
				else if(cull_mode == "back")
					desc.cull_mode = cull_mode_back;
				else
					throw std::invalid_argument(cull_mode + "は認識できません");
			}
		}
	}

	for(const json_file::values_t& state : values["input_layout"])
	{
		if(auto it = state.find("name"); it != state.end())
		{
			auto& desc = input_layout_descs[string(it->second[0])];
			if(auto it = state.find("topology"); it != state.end())
			{
				const string& topology = it->second[0];
				if(topology == "point_list")
					desc.topology = topology_type_point_list;
				else if(topology == "line_list")
					desc.topology = topology_type_line_list;
				else if(topology == "line_strip")
					desc.topology = topology_type_line_strip;
				else if(topology == "triangle_list")
					desc.topology = topology_type_triangle_list;
				else if(topology == "triangle_strip")
					desc.topology = topology_type_triangle_strip;
				else
					throw std::invalid_argument(topology + "は認識できません");
			}
			if(auto it = state.find("element"); it != state.end())
			{
				desc.num_elements = 0;
				for(const json_file::values_t& values : it->second)
				{
					const uint i = desc.num_elements++;
					if(auto it = values.find("name"); it != values.end())
						desc.element_descs[i].semantic_name = string(it->second[0]);
					if(auto it = values.find("index"); it != values.end())
						desc.element_descs[i].semantic_index = uint8_t(it->second[0]);
					if(auto it = values.find("slot"); it != values.end())
						desc.element_descs[i].slot = uint8_t(it->second[0]);
					if(auto it = values.find("offset"); it != values.end())
						desc.element_descs[i].offset = uint8_t(it->second[0]);

					if(auto it = values.find("format"); it != values.end())
					{
						const string& format = it->second[0];
						if(format == "r32g32b32a32_float")
							desc.element_descs[i].format = texture_format_r32g32b32a32_float;
						else if(format == "r32g32b32a32_uint")
							desc.element_descs[i].format = texture_format_r32g32b32a32_uint;
						else if(format == "r32g32b32a32_sint")
							desc.element_descs[i].format = texture_format_r32g32b32a32_sint;
						else if(format == "r32g32b32_float")
							desc.element_descs[i].format = texture_format_r32g32b32_float;
						else if(format == "r32g32b32_uint")
							desc.element_descs[i].format = texture_format_r32g32b32_uint;
						else if(format == "r32g32b32_sint")
							desc.element_descs[i].format = texture_format_r32g32b32_sint;
						else if(format == "r16g16b16a16_float")
							desc.element_descs[i].format = texture_format_r16g16b16a16_float;
						else if(format == "r16g16b16a16_unorm")
							desc.element_descs[i].format = texture_format_r16g16b16a16_unorm;
						else if(format == "r16g16b16a16_uint")
							desc.element_descs[i].format = texture_format_r16g16b16a16_uint;
						else if(format == "r16g16b16a16_snorm")
							desc.element_descs[i].format = texture_format_r16g16b16a16_snorm;
						else if(format == "r16g16b16a16_sint")
							desc.element_descs[i].format = texture_format_r16g16b16a16_sint;
						else if(format == "r32g32_float")
							desc.element_descs[i].format = texture_format_r32g32_float;
						else if(format == "r32g32_uint")
							desc.element_descs[i].format = texture_format_r32g32_uint;
						else if(format == "r32g32_sint")
							desc.element_descs[i].format = texture_format_r32g32_sint;
						else if(format == "r10g10b10a2_unorm")
							desc.element_descs[i].format = texture_format_r10g10b10a2_unorm;
						else if(format == "r10g10b10a2_uint")
							desc.element_descs[i].format = texture_format_r10g10b10a2_uint;
						else if(format == "r11g11b10_float")
							desc.element_descs[i].format = texture_format_r11g11b10_float;
						else if(format == "r8g8b8a8_unorm")
							desc.element_descs[i].format = texture_format_r8g8b8a8_unorm;
						else if(format == "r8g8b8a8_uint")
							desc.element_descs[i].format = texture_format_r8g8b8a8_uint;
						else if(format == "r8g8b8a8_snorm")
							desc.element_descs[i].format = texture_format_r8g8b8a8_snorm;
						else if(format == "r8g8b8a8_sint")
							desc.element_descs[i].format = texture_format_r8g8b8a8_sint;
						else if(format == "r16g16_float")
							desc.element_descs[i].format = texture_format_r16g16_float;
						else if(format == "r16g16_unorm")
							desc.element_descs[i].format = texture_format_r16g16_unorm;
						else if(format == "r16g16_uint")
							desc.element_descs[i].format = texture_format_r16g16_uint;
						else if(format == "r16g16_snorm")
							desc.element_descs[i].format = texture_format_r16g16_snorm;
						else if(format == "r16g16_sint")
							desc.element_descs[i].format = texture_format_r16g16_sint;
						else if(format == "r32_float")
							desc.element_descs[i].format = texture_format_r32_float;
						else if(format == "r32_uint")
							desc.element_descs[i].format = texture_format_r32_uint;
						else if(format == "r32_sint")
							desc.element_descs[i].format = texture_format_r32_sint;
						else if(format == "r8g8_unorm")
							desc.element_descs[i].format = texture_format_r8g8_unorm;
						else if(format == "r8g8_uint")
							desc.element_descs[i].format = texture_format_r8g8_uint;
						else if(format == "r8g8_snorm")
							desc.element_descs[i].format = texture_format_r8g8_snorm;
						else if(format == "r8g8_sint")
							desc.element_descs[i].format = texture_format_r8g8_sint;
						else if(format == "r16_float")
							desc.element_descs[i].format = texture_format_r16_float;
						else if(format == "r16_unorm")
							desc.element_descs[i].format = texture_format_r16_unorm;
						else if(format == "r16_uint")
							desc.element_descs[i].format = texture_format_r16_uint;
						else if(format == "r16_snorm")
							desc.element_descs[i].format = texture_format_r16_snorm;
						else if(format == "r16_sint")
							desc.element_descs[i].format = texture_format_r16_sint;
						else if(format == "r8_unorm")
							desc.element_descs[i].format = texture_format_r8_unorm;
						else if(format == "r8_uint")
							desc.element_descs[i].format = texture_format_r8_uint;
						else if(format == "r8_snorm")
							desc.element_descs[i].format = texture_format_r8_snorm;
						else if(format == "r8_sint")
							desc.element_descs[i].format = texture_format_r8_sint;
						else
							throw std::invalid_argument(format + "は認識できません");
					}
				}
			}
		}
	}

	if(mp_compiler == nullptr)
		mp_compiler = device.create_shader_compiler();

	unordered_map<size_t, shader_ptr> shader_ptrs;
	auto compile_shader = [&](const string& filename, const string& entry_point, const string& target, const string& options)
	{
		std::vector<char> concat;
		concat.reserve(filename.size() + entry_point.size() + target.size() + options.size());
		concat.insert(concat.end(), filename.begin(), filename.end());
		concat.insert(concat.end(), entry_point.begin(), entry_point.end());
		concat.insert(concat.end(), target.begin(), target.end());
		concat.insert(concat.end(), options.begin(), options.end());

		auto& p_shader = shader_ptrs.try_emplace(calc_hash(concat.data(), concat.size())).first->second;
		if(p_shader == nullptr)
		{
			auto wfilename = utf8_to_utf16(filename);
			p_shader = mp_compiler->compile(wfilename, utf8_to_utf16(entry_point), utf8_to_utf16(target), utf8_to_utf16(options), dependent);
			dependent.emplace(format(wfilename));
		}
		return p_shader;
	};

	unordered_map<string, pipeline_state_ptr> ps_ptrs;
	if(auto it = values.find("definition"); it != values.end())
	{
		for(const json_file::values_t& values : it->second)
		{
			if(auto it = values.find("name"); it != values.end())
			{
				pipeline_state_ptr p_ps;
				const string& name = it->second[0];
				if(auto it = values.find("cs"); it != values.end())
				{
					shader_ptr cs = compile_shader(it->second[0], it->second[1], "cs_6_8", it->second[2]); 
					p_ps = device.create_compute_pipeline_state(name, std::move(cs));
				}
				else
				{
					shader_ptr vs;
					if(auto it = values.find("vs"); it != values.end())
						vs = compile_shader(it->second[0], it->second[1], "vs_6_8", it->second[2]);

					shader_ptr ps;
					if(auto it = values.find("ps"); it != values.end())
						ps = compile_shader(it->second[0], it->second[1], "ps_6_8", it->second[2]);
					
					input_layout_desc il;
					if(auto it = values.find("il"); it != values.end())
						il = input_layout_descs[it->second[0].string()];

					rasterizer_desc rs;
					if(auto it = values.find("rs"); it != values.end())
						rs = rasterizer_descs[it->second[0].string()];

					depth_stencil_desc dss;
					if(auto it = values.find("dss"); it != values.end())
						dss = depth_stencil_descs[it->second[0].string()];

					blend_desc bs;
					if(auto it = values.find("bs"); it != values.end())
						bs = blend_descs[it->second[0].string()];

					p_ps = device.create_graphics_pipeline_state(name, std::move(vs), std::move(ps), il, rs, dss, bs);
				}
				ps_ptrs[it->second[0].string()] = std::move(p_ps);
			}
		}
	}
	if(auto it = values.find("definitions"); it != values.end())
	{
		for(const json_file::values_t& definitions : it->second)
		{
			if(auto it = definitions.find("direct_product"); it != definitions.end())
			{
				using params_t = unordered_map<string, array<string, 3>>;
				const vector<json_file::value_t> &direct_product = it->second;
				auto recurse = [&](const uint depth, const params_t &parent_params, auto* This) -> void
				{
					if(depth >= direct_product.size())
					{
						auto& params = parent_params;
						if(auto it = params.find("name"); it != params.end())
						{
							pipeline_state_ptr p_ps;
							const string& name = it->second[0];
							if(auto it = params.find("cs"); it != params.end())
							{
								shader_ptr cs = compile_shader(it->second[0], it->second[1], "cs_6_8", it->second[2]); 
								p_ps = device.create_compute_pipeline_state(name, std::move(cs));
							}
							else
							{
								shader_ptr vs;
								if(auto it = params.find("vs"); it != params.end())
									vs = compile_shader(it->second[0], it->second[1], "vs_6_8", it->second[2]);

								shader_ptr ps;
								if(auto it = params.find("ps"); it != params.end())
									ps = compile_shader(it->second[0], it->second[1], "ps_6_8", it->second[2]);
					
								input_layout_desc il;
								if(auto it = params.find("il"); it != params.end())
									il = input_layout_descs[it->second[0]];

								rasterizer_desc rs;
								if(auto it = params.find("rs"); it != params.end())
									rs = rasterizer_descs[it->second[0]];

								depth_stencil_desc dss;
								if(auto it = params.find("dss"); it != params.end())
									dss = depth_stencil_descs[it->second[0]];

								blend_desc bs;
								if(auto it = params.find("bs"); it != params.end())
									bs = blend_descs[it->second[0]];

								p_ps = device.create_graphics_pipeline_state(name, std::move(vs), std::move(ps), il, rs, dss, bs);
							}
							ps_ptrs[it->second[0]] = std::move(p_ps);
						}
						return;
					}

					if(auto it = direct_product[depth].values().find("definition"); it != direct_product[depth].values().end())
					{
						for(const json_file::values_t& definition : it->second)
						{
							auto load_shader_param = [&](params_t& params, const string& key)
							{
								if(auto it = definition.find(key); it != definition.end())
								{
									auto& param = params.try_emplace(key).first->second;
									if(const string& s = it->second[0]; not(s.empty()))
										param[0] = s;
									if(const string& s = it->second[1]; not(s.empty()))
										param[1] = s;
									if(const string& s = it->second[2]; not(s.empty()))
										(param[2] += " ") += s;
								}
							};
							auto load_state = [&](params_t& params, const string& key)
							{
								if(auto it = definition.find(key); it != definition.end())
									params[key][0] = it->second[0].string();
							};

							params_t params = parent_params;
							load_shader_param(params, "vs");
							load_shader_param(params, "ps");
							load_shader_param(params, "cs");
							load_state(params, "il");
							load_state(params, "rs");
							load_state(params, "dss");
							load_state(params, "bs");

							if(auto it = definition.find("name"); it != definition.end())
								params["name"][0] += it->second[0].string();

							(*This)(depth + 1, params, This);
						}
					}
				};
				recurse(0, params_t{}, &recurse);
			}
		}
	}
	return std::make_shared<shader_file>(std::move(ps_ptrs));
}

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace render

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////
