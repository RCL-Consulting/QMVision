#include <vsg/all.h>
#include <vsgXchange/all.h>
#include <iostream>

#include "GeomBasics.h"
#include "QMorph.h"
#include "Node.h"
#include "Quad.h"
#include "Element.h"
#include "Edge.h"

#include <filesystem>

std::string VERT{ R"(
#version 450
layout(push_constant) uniform PushConstants { mat4 projection; mat4 modelView; };
layout(location = 0) in vec3 vertex;
layout(location = 1) in vec4 color;
layout(location = 2) out vec4 rasterColor;
void main()
{
    rasterColor = color;
    gl_Position = projection * modelView * vec4(vertex, 1.0);
}
)" };

std::string FRAG{ R"(
#version 450
layout(location = 2) in vec4 rasterColor;
layout(location = 0) out vec4 fragmentColor;
void main()
{
    fragmentColor = rasterColor;
}
)" };

template<typename T>
vsg::ref_ptr<vsg::VertexDraw> createEdgeOverlay( const ArrayList<std::shared_ptr<T>>& elements )
{
    std::set<std::pair<const std::shared_ptr<Node>, const std::shared_ptr<Node>>> uniqueEdges;
    for ( const auto& elem : elements )
    {
        for ( const auto& edge : elem->edgeList )
        {
            const auto& n1 = edge->leftNode;
            const auto& n2 = edge->rightNode;
            if ( n1 && n2 )
            {
                // Sort the pair to ensure uniqueness regardless of direction
                auto ordered = std::minmax( n1, n2 );
                uniqueEdges.insert( ordered );
            }
        }
    }

    auto positions = vsg::vec3Array::create( uniqueEdges.size()*2 );
    size_t Index = 0;
    for ( auto [n1, n2] : uniqueEdges )
    {
        positions->set( Index++, vsg::vec3( n1->x, n1->y, 0.0 ) );
        positions->set( Index++, vsg::vec3( n2->x, n2->y, 0.0 ) );
    }

    auto draw = vsg::VertexDraw::create();
    draw->assignArrays( { positions } );
    draw->vertexCount = positions->size();
    draw->instanceCount = 1;

    return draw;
}

//vsg::ref_ptr<vsg::VertexDraw> createEdgeOverlay( const ArrayList<std::shared_ptr<Triangle>>& elements )
//{
//    std::set<std::pair<const std::shared_ptr<Node>, const std::shared_ptr<Node>>> uniqueEdges;
//    for ( const auto& elem : elements )
//    {
//        for ( const auto& edge : elem->edgeList )
//        {
//            const auto& n1 = edge->leftNode;
//            const auto& n2 = edge->rightNode;
//            if ( n1 && n2 )
//            {
//                // Sort the pair to ensure uniqueness regardless of direction
//                auto ordered = std::minmax( n1, n2 );
//                uniqueEdges.insert( ordered );
//            }
//        }
//    }
//
//    auto positions = vsg::vec3Array::create( uniqueEdges.size() * 2 );
//    size_t Index = 0;
//    for ( auto [n1, n2] : uniqueEdges )
//    {
//        positions->set( Index++, vsg::vec3( n1->x, n1->y, 0.0 ) );
//        positions->set( Index++, vsg::vec3( n2->x, n2->y, 0.0 ) );
//    }
//
//    auto draw = vsg::VertexDraw::create();
//    draw->assignArrays( { positions } );
//    draw->vertexCount = positions->size();
//    draw->instanceCount = 1;
//
//    return draw;
//}

vsg::ref_ptr<vsg::BindGraphicsPipeline> createLinePipeline()
{
    vsg::DescriptorSetLayouts descriptorSetLayouts{};
    vsg::PushConstantRanges pushConstantRanges{ {VK_SHADER_STAGE_VERTEX_BIT, 0, 128} };
    auto pipelineLayout = vsg::PipelineLayout::create(
        descriptorSetLayouts, pushConstantRanges );

    auto vert = vsg::ShaderStage::create(
        VK_SHADER_STAGE_VERTEX_BIT,
        "main",
        R"(
            #version 450
            layout(location = 0) in vec3 vertex;
            layout(push_constant) uniform PushConstants { mat4 projection; mat4 modelView; };
            void main()
            {
                gl_Position = projection * modelView * vec4(vertex, 1.0);
            }
        )"
    );

    auto frag = vsg::ShaderStage::create(
        VK_SHADER_STAGE_FRAGMENT_BIT,
        "main",
        R"(
            #version 450
            layout(location = 0) out vec4 outColor;
            void main()
            {
                outColor = vec4(1.0, 1.0, 1.0, 1.0); // white lines
            }
        )"
    );

    auto vertexInput = vsg::VertexInputState::create();
    vertexInput->vertexBindingDescriptions.push_back( { 0, sizeof( vsg::vec3 ), VK_VERTEX_INPUT_RATE_VERTEX } );
    vertexInput->vertexAttributeDescriptions.push_back( { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 } );

    auto inputAssembly = vsg::InputAssemblyState::create();
    inputAssembly->topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;

    auto pipeline = vsg::GraphicsPipeline::create(
        pipelineLayout,
        vsg::ShaderStages{ vert, frag },
        vsg::GraphicsPipelineStates{
            vertexInput,
            inputAssembly,
            vsg::RasterizationState::create(),
            vsg::ColorBlendState::create(),
            vsg::MultisampleState::create(),
            vsg::DepthStencilState::create()
        }
    );

    return vsg::BindGraphicsPipeline::create( pipeline );
}

vsg::ref_ptr<vsg::BindGraphicsPipeline> createBindGraphicsPipeline()
{
    vsg::DescriptorSetLayouts descriptorSetLayouts{};
    vsg::PushConstantRanges pushConstantRanges{ {VK_SHADER_STAGE_VERTEX_BIT, 0, 128} };
    auto pipelineLayout = vsg::PipelineLayout::create(
        descriptorSetLayouts, pushConstantRanges );

    auto vertexShader = vsg::ShaderStage::create( VK_SHADER_STAGE_VERTEX_BIT, "main", VERT );
    auto fragmentShader = vsg::ShaderStage::create( VK_SHADER_STAGE_FRAGMENT_BIT, "main", FRAG );

    auto vertexInput = vsg::VertexInputState::create();
    vertexInput->vertexBindingDescriptions = {
        {0, sizeof( vsg::vec3 ), VK_VERTEX_INPUT_RATE_VERTEX},
        {1, sizeof( vsg::vec4 ), VK_VERTEX_INPUT_RATE_VERTEX}
    };
    vertexInput->vertexAttributeDescriptions = {
        {0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0},
        {1, 1, VK_FORMAT_R32G32B32A32_SFLOAT, 0}
    };

    auto inputAssembly = vsg::InputAssemblyState::create();
    inputAssembly->topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    auto rasterizationState = vsg::RasterizationState::create();
    rasterizationState->cullMode = VK_CULL_MODE_NONE;
    rasterizationState->depthBiasEnable = VK_TRUE;
    rasterizationState->depthBiasConstantFactor = -1.0f;  // Similar to glPolygonOffset(1.0, 1.0)
    rasterizationState->depthBiasSlopeFactor = -1.0f;

    auto graphicsPipeline = vsg::GraphicsPipeline::create(
        pipelineLayout,
        vsg::ShaderStages{ vertexShader, fragmentShader },
        vsg::GraphicsPipelineStates{
            vertexInput,
            inputAssembly,
            rasterizationState,
            vsg::ColorBlendState::create(),
            vsg::MultisampleState::create(),
            vsg::DepthStencilState::create()
        }
    );

    return vsg::BindGraphicsPipeline::create( graphicsPipeline );
}

//vsg::ref_ptr<vsg::VertexIndexDraw> createDrawFromMesh( const ArrayList<std::shared_ptr<Triangle>>& triangles )
//{
//    std::map<std::shared_ptr<Node>, uint16_t> nodeToIndex;
//    std::vector<std::shared_ptr<Node>> orderedNodes;
//    std::vector<std::array<float, 4>> orderedColors;
//    std::vector<uint16_t> tempIndices;
//
//    std::array<float, 4> TriColor = { 0.8f, 0.2f, 0.2f, 1.0f };
//
//    for ( const auto& T : triangles )
//    {        
//		std::vector<std::shared_ptr<Node>> triNodes;
//		for ( const auto& edge : T->edgeList )
//		{
//			triNodes.push_back( edge->leftNode );
//		}
//		// Triangle: 0, 1, 2
//		for ( int idx : {0, 1, 2} )
//		{
//			const auto node = triNodes[idx];
//			if ( nodeToIndex.find( node ) == nodeToIndex.end() )
//			{
//				nodeToIndex[node] = static_cast<uint16_t>(orderedNodes.size());
//				orderedNodes.push_back( node );
//				orderedColors.push_back( TriColor );
//			}
//			tempIndices.push_back( nodeToIndex[node] );
//		}
//    }
//
//    auto positions = vsg::vec3Array::create( orderedNodes.size() );
//    auto colors = vsg::vec4Array::create( orderedNodes.size() );
//    auto indices = vsg::ushortArray::create( tempIndices.size() );
//
//    for ( size_t i = 0; i < orderedNodes.size(); ++i )
//    {
//        const auto node = orderedNodes[i];
//        (*positions)[i] = vsg::vec3( node->x, node->y, 0.0 );
//        const auto& color = orderedColors[i];
//        (*colors)[i] = vsg::vec4( color[0], color[1], color[2], color[3] );
//    }
//
//    for ( size_t i = 0; i < tempIndices.size(); ++i )
//    {
//        (*indices)[i] = tempIndices[i];
//    }
//
//    vsg::DataList vertexArrays{ positions, colors };
//    auto draw = vsg::VertexIndexDraw::create();
//    draw->assignArrays( vertexArrays );
//    draw->assignIndices( indices );
//    draw->indexCount = indices->size();
//    draw->instanceCount = 1;
//    return draw;
//}

template<typename T>
vsg::ref_ptr<vsg::VertexIndexDraw>
createDrawFromMesh(const ArrayList<std::shared_ptr<T>>& elements)
{
    std::map<std::shared_ptr<Node>, uint16_t> nodeToIndex;
    std::vector<std::shared_ptr<Node>> orderedNodes;
	std::vector<std::array<float, 4>> orderedColors;
    std::vector<uint16_t> tempIndices;

	std::array<float, 4> QuadColor = { 0.2f, 0.7f, 1.0f, 1.0f };
	std::array<float, 4> TriColor = { 0.8f, 0.2f, 0.2f, 1.0f };

    for ( const auto& elem : elements )
    {
        if ( auto Q = std::dynamic_pointer_cast<Quad>(elem) )
        {
            std::vector<std::shared_ptr<Node>> quadNodes;
            std::set<std::shared_ptr<Node>> seen;
            auto FirstNode = elem->edgeList[0]->leftNode;
            auto Current = FirstNode;
            quadNodes.emplace_back( FirstNode );
            while ( (Current = Q->nextCCWNode( Current )) != FirstNode )
                quadNodes.emplace_back( Current );

            if ( quadNodes.size() != 4 ) continue; // skip non-quads

            // Triangle 1: 0, 1, 2
            for ( int idx : {0, 1, 2} )
            {
                const auto node = quadNodes[idx];
                if ( nodeToIndex.find( node ) == nodeToIndex.end() )
                {
                    nodeToIndex[node] = static_cast<uint16_t>(orderedNodes.size());
                    orderedNodes.push_back( node );
					orderedColors.push_back( QuadColor );
                }
                tempIndices.push_back( nodeToIndex[node] );
            }

            // Triangle 2: 0, 2, 3
            for ( int idx : {0, 2, 3} )
            {
                const auto node = quadNodes[idx];
                if ( nodeToIndex.find( node ) == nodeToIndex.end() )
                {
                    nodeToIndex[node] = static_cast<uint16_t>(orderedNodes.size());
                    orderedNodes.push_back( node );
                    orderedColors.push_back( QuadColor );
                }
                tempIndices.push_back( nodeToIndex[node] );
            }
        }
		else if ( auto T = std::dynamic_pointer_cast<Triangle>(elem) )
		{
			std::vector<std::shared_ptr<Node>> triNodes;
			for ( const auto& edge : elem->edgeList )
			{
				if ( std::find( triNodes.begin(), triNodes.end(), edge->leftNode ) == triNodes.end() )
					triNodes.push_back( edge->leftNode );
                if ( std::find( triNodes.begin(), triNodes.end(), edge->rightNode ) == triNodes.end() )
					triNodes.push_back( edge->rightNode );
			}

			// Triangle: 0, 1, 2
			for ( int idx : {0, 1, 2} )
			{
				const auto node = triNodes[idx];
				if ( nodeToIndex.find( node ) == nodeToIndex.end() )
				{
					nodeToIndex[node] = static_cast<uint16_t>(orderedNodes.size());
					orderedNodes.push_back( node );
                    orderedColors.push_back( TriColor );
				}
				tempIndices.push_back( nodeToIndex[node] );
			}
		}
         
    }

    auto positions = vsg::vec3Array::create( orderedNodes.size() );
    auto colors = vsg::vec4Array::create( orderedNodes.size() );
    auto indices = vsg::ushortArray::create( tempIndices.size() );

    for ( size_t i = 0; i < orderedNodes.size(); ++i )
    {
        const auto node = orderedNodes[i];
        (*positions)[i] = vsg::vec3( node->x, node->y, 0.0 );
		const auto& color = orderedColors[i];
		(*colors)[i] = vsg::vec4( color[0], color[1], color[2], color[3] );
    }

    for ( size_t i = 0; i < tempIndices.size(); ++i )
    {
        (*indices)[i] = tempIndices[i];
    }

    vsg::DataList vertexArrays{ positions, colors };
    auto draw = vsg::VertexIndexDraw::create();
    draw->assignArrays( vertexArrays );
	draw->assignIndices( indices );
    draw->indexCount = indices->size();
    draw->instanceCount = 1;
    return draw;
}

int
main( int argc, 
      char* argv[] )
{
    if (argc < 2)
    {
        std::cout << "Usage: QuadMindConsole <input file>\n";
        return 1;
    }

	std::string inputFilename = argv[1];
	std::filesystem::path inputPath( inputFilename );

	GeomBasics::clearLists();

	GeomBasics::setParams( inputPath.filename().string(), inputPath.parent_path().string(), false, false );
	GeomBasics::loadMesh();
	GeomBasics::findExtremeNodes();

	auto Morph = std::make_shared<QMorph>();
    Morph->init( -1, 0.6, false );
	Morph->run();

    auto viewer = vsg::Viewer::create();
	auto windowTraits = vsg::WindowTraits::create();
	auto window = vsg::Window::create( windowTraits );
	viewer->addWindow( window );

    auto options = vsg::Options::create();
    options->paths = vsg::getEnvPaths( "VSG_FILE_PATH" );
#ifdef vsgXchange_all
    // add vsgXchange's support for reading and writing 3rd party file formats
    options->add( vsgXchange::all::create() );
#endif

    auto font = vsg::read_cast<vsg::Font>( "fonts/times.vsgb", options );
    if ( !font )
    {
        std::cerr << "Could not load font.\n";
        return {};
    } 
    font->setValue( "fontSize", 1.0f );

    // Compute bounds from nodes
    vsg::dvec3 minBounds( DBL_MAX, DBL_MAX, 0.0 );
    vsg::dvec3 maxBounds( -DBL_MAX, -DBL_MAX, 0.0 );
    for ( const auto& node : GeomBasics::nodeList )
    {
        minBounds.x = std::min( minBounds.x, node->x );
        minBounds.y = std::min( minBounds.y, node->y );
        maxBounds.x = std::max( maxBounds.x, node->x );
        maxBounds.y = std::max( maxBounds.y, node->y );
    }

    double pad = 0.1 * std::max( maxBounds.x - minBounds.x, maxBounds.y - minBounds.y );
    vsg::dvec3 paddedMin = minBounds - vsg::dvec3( pad, pad, 0.0 );
    vsg::dvec3 paddedMax = maxBounds + vsg::dvec3( pad, pad, 0.0 );
    vsg::dvec3 center = (paddedMin + paddedMax) * 0.5;

    // Use perspective projection for proper zooming
    double aspect = static_cast<double>(window->extent2D().width) / window->extent2D().height;
    auto projection = vsg::Perspective::create( 30.0, aspect, 0.1, 1000.0 );

    auto lookAt = vsg::LookAt::create(
        center + vsg::dvec3( 0.0, 0.0, 5.0 ), // eye above the mesh
        center,                             // look at center
        vsg::dvec3( 0.0, 1.0, 0.0 ) );         // up direction

    auto viewportState = vsg::ViewportState::create( window->extent2D() );
    auto camera = vsg::Camera::create( projection, lookAt, viewportState );

    auto stateGroup = vsg::StateGroup::create();

    if ( GeomBasics::elementList.size() )
    {
        auto quadGroup = vsg::StateGroup::create();
        quadGroup->add( createBindGraphicsPipeline() );
        quadGroup->addChild( createDrawFromMesh( GeomBasics::elementList ) );
        stateGroup->addChild( quadGroup );

        auto quadLineGroup = vsg::StateGroup::create();
        quadLineGroup->add( createLinePipeline() );
        quadLineGroup->addChild( createEdgeOverlay( GeomBasics::elementList ) );
        stateGroup->addChild( quadLineGroup );
    }

    if ( GeomBasics::triangleList.size() )
    {
        auto triangleGroup = vsg::StateGroup::create();
        triangleGroup->add( createBindGraphicsPipeline() );
        triangleGroup->addChild( createDrawFromMesh( GeomBasics::triangleList ) );
        stateGroup->addChild( triangleGroup );

        auto triangleLineGroup = vsg::StateGroup::create();
        triangleLineGroup->add( createLinePipeline() );
        triangleLineGroup->addChild( createEdgeOverlay( GeomBasics::triangleList ) );
        stateGroup->addChild( triangleLineGroup );
    } 

    if ( GeomBasics::nodeList.size() )
    {
        auto textGroup = vsg::StateGroup::create();

        auto shaderSet = options->shaderSets["text"] =
            vsg::createTextShaderSet( options );

        // create a DepthStencilState, disable depth test and add this to the
        // ShaderSet::defaultGraphicsPipelineStates container so it's used when
        // setting up the TextGroup subgraph
        auto depthStencilState = vsg::DepthStencilState::create();
        depthStencilState->depthTestEnable = VK_FALSE;
        shaderSet->defaultGraphicsPipelineStates.push_back( depthStencilState );

        for ( size_t i = 0; i < GeomBasics::nodeList.size(); ++i )
        {
            const auto& node = GeomBasics::nodeList.get( i );

            auto layout = vsg::StandardLayout::create();
            layout->horizontalAlignment = vsg::StandardLayout::CENTER_ALIGNMENT;
            layout->position = vsg::vec3( node->x, node->y, 0.0 );
            float textHeight = 0.05f;

            layout->horizontal = vsg::vec3( textHeight, 0.0f, 0.0f );
            layout->vertical = vsg::vec3( 0.0f, textHeight, 0.0f );
            layout->color = vsg::vec4( 1.0, 1.0, 1.0, 1.0 );
            layout->outlineWidth = 0.1f;
            layout->billboard = true;

            auto label = vsg::Text::create();
            label->font = font;
            label->text = vsg::stringValue::create( std::to_string( node->GetNumber() ) );
            label->layout = layout;
            label->setup( 0, options );

            textGroup->addChild( label );
        }

        stateGroup->addChild( textGroup );
    }

    auto view = vsg::View::create();
    view->camera = camera;
    view->addChild( stateGroup );

    viewer->addEventHandler( vsg::CloseHandler::create( viewer ) );
    viewer->addEventHandler( vsg::Trackball::create( camera ) );

    auto renderGraph = vsg::RenderGraph::create( window, view );
    auto commandGraph = vsg::CommandGraph::create( window );
    commandGraph->addChild( renderGraph );

    viewer->assignRecordAndSubmitTaskAndPresentation( { commandGraph } );
    viewer->compile();

    while ( viewer->advanceToNextFrame() )
    {
        viewer->handleEvents();
        viewer->update();
        viewer->recordAndSubmit();
        viewer->present();
    }

	return 0;
}

