地形色彩标准数据结构示例：
{
    "splat" : 
    {
        "0,0" : 
        {
            "splat" : "Assets/...",
            "index" : "Assets/..."
        },
        "0,1" :
        {
            "splat" : "Assets/...",
            "index" : "Assets/..."
        }
    },
    "texture" : 
    {
        "0" : 
        {
            "albedo" : "Assets/...",
            "normal" : "Assets/..."
        },
        "1" : 
        {
            "albedo" : "Assets/...",
            "normal" : "Assets/..."
        }
    }
}
地形高度图数据结构示例：
{
    "0,0" : "Assets/..."
}
地形高度包围盒结构示例：
{
    "property" : 
    {
        "covered_size" : 8192,
        "max_resolution" : 256,
        "mip" : 5
    },
    "datas" : 
    {
        "0" : "Assets/..."
    }
}
投影Shader变量属性：
_Params: ProjectParams
_MainTex: global desc
_IndexMap: global desc
_DescriptorIndexBufer: albedo + normal structuredbuffer

程序架构：
VirtualTexture: 虚拟纹理基础逻辑组件
TerrainDrawer: 网格渲染组件
VirtualTextureData：硬盘资源读写和加载类型
TerrainMainLogic: VT着色贴图四叉树逻辑管理
依赖关系： 
TerrainDrawer->VirtualTexture
TerrainMainLogic->VirtualTexture, VirtualTextureData
