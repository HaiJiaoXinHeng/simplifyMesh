
#include "stdmix.h"
#include "mixio.h"
#include "qslim.h"
#include "MxTimer.h"
#include "MxSMF.h"
#include "MxPropSlim.h"
#include"MxStdSlim.h"
#pragma comment(lib,"MixSrc.lib")
#pragma comment(lib,"MixSrcd.lib")
#pragma comment(lib,"JpegLib.lib")
// Configuration variables and initial values
//
unsigned int face_target = 0;
bool will_use_fslim = false;
int placement_policy = MX_PLACE_OPTIMAL;
double boundary_weight = 1000.0;
int weighting_policy = MX_WEIGHT_AREA;
bool will_record_history = false;
double compactness_ratio = 0.0;
double meshing_penalty = 1.0;
bool will_join_only = false;
bool be_quiet = false;
OutputFormat output_format = SMF;
char *output_filename = NULL;

// Globally visible variables
//
MxSMFReader *smf = NULL;
MxStdModel *m = NULL;
MxStdModel *m_orig = NULL;
MxQSlim *slim = NULL;//这是哪里定义的
MxEdgeQSlim *eslim = NULL;
MxFaceQSlim *fslim = NULL;
QSlimLog *history = NULL;
MxDynBlock<MxEdge> *target_edges = NULL;

const char *slim_copyright_notice =
"Copyright (C) 1998-2002 Michael Garland.  See \"COPYING.txt\" for details.";

const char *slim_version_string = "2.1";

static ostream *output_stream = NULL;

static
bool qslim_smf_hook(char *op, int, char *argv[], MxStdModel& m)
{
    if( streq(op, "e") )
    {
	if( !target_edges )
	    target_edges = new MxDynBlock<MxEdge>(m.vert_count() * 3);

	MxEdge& e = target_edges->add();

	e.v1 = atoi(argv[0]) - 1;
	e.v2 = atoi(argv[1]) - 1;

	return true;
    }

    return false;
}

bool (*unparsed_hook)(char *, int, char*[], MxStdModel&) = qslim_smf_hook;

void slim_print_banner(ostream& out)
{
    out << "QSlim surface simplification software." << endl
	<< "Version " << slim_version_string << " "
	<< "[Built " << __DATE__ << "]." << endl
	<< slim_copyright_notice << endl;
}

void slim_init()
{
    if( !slim )
    {
	if( will_use_fslim )
	    slim = fslim = new MxFaceQSlim(*m);
	else
	    slim = eslim = new MxEdgeQSlim(*m);
    }
    else
    {
	if( will_use_fslim )
	    fslim = (MxFaceQSlim *)slim;
	else
	    eslim = (MxEdgeQSlim *)slim;
    }

    slim->placement_policy = placement_policy;
    slim->boundary_weight = boundary_weight;
    slim->weighting_policy = weighting_policy;
    slim->compactness_ratio = compactness_ratio;
    slim->meshing_penalty = meshing_penalty;
    slim->will_join_only = will_join_only;

    if( eslim && target_edges )
    {
	eslim->initialize(*target_edges, target_edges->length());
    }
    else
	slim->initialize();

    if( will_record_history )
    {
	if( !eslim )
	    mxmsg_signal(MXMSG_WARN,
			 "History only available for edge contractions.");
	else
	{
	    history = new QSlimLog(100);
	    eslim->contraction_callback = slim_history_callback;
	}
    }
}

#define CLEANUP(x)  if(x) { delete x; x=NULL; }

void slim_cleanup()
{
    CLEANUP(smf);
    CLEANUP(m);
    CLEANUP(slim);
    eslim = NULL;
    fslim = NULL;
    CLEANUP(history);
    CLEANUP(target_edges);
    if( output_stream != &cout )
    	CLEANUP(output_stream);
}

static
void setup_output()
{
    if( !output_stream )
    {
	if( output_filename )
	    output_stream = new ofstream(output_filename);
	else
	    output_stream = &cout;
    }
}

bool select_output_format(const char *fmt)
{
    bool h = false;

    if     ( streq(fmt, "mmf") ) { output_format = MMF; h = true; }
    else if( streq(fmt, "pm")  ) { output_format = PM;  h = true; }
    else if( streq(fmt, "log") ) { output_format = LOG; h = true; }
    else if( streq(fmt, "smf") ) output_format = SMF;
    else if( streq(fmt, "iv")  ) output_format = IV;
    else if( streq(fmt, "vrml")) output_format = VRML;
    else return false;

    if( h ) will_record_history = true;

    return true;
}

void output_preamble()
{
    if( output_format==MMF || output_format==LOG )
	output_current_model();
}

void output_current_model()
{
    setup_output();

    MxSMFWriter writer;
    writer.write(*output_stream, *m);
}

static
void cleanup_for_output()
{
    // First, mark stray vertices for removal
    //
    for(uint i=0; i<m->vert_count(); i++)
	if( m->vertex_is_valid(i) && m->neighbors(i).length() == 0 )
	    m->vertex_mark_invalid(i);

	// Compact vertex array so only valid vertices remain
    m->compact_vertices();
}

void output_final_model()
{
    setup_output();

    switch( output_format )
    {
    case MMF:
	output_regressive_mmf(*output_stream);
	break;

    case LOG:
	output_regressive_log(*output_stream);
	break;

    case PM:
	output_progressive_pm(*output_stream);
	break;

    case IV:
	cleanup_for_output();
	output_iv(*output_stream);
	break;

    case VRML:
	cleanup_for_output();
	output_vrml(*output_stream);
	break;

    case SMF:
	cleanup_for_output();
	output_current_model();
	break;
    }


}

void input_file(const char *filename)
{
    if( streq(filename, "-") )
	smf->read(cin, m);
    else
    {
	ifstream in(filename);
	if( !in.good() )
	    mxmsg_signal(MXMSG_FATAL, "Failed to open input file", filename);
	smf->read(in, m);
	in.close();
    }
}

static
MxDynBlock<char*> files_to_include(2);

void defer_file_inclusion(char *filename)
{
    files_to_include.add(filename);
}

void include_deferred_files()
{
    for(uint i=0; i<files_to_include.length(); i++)
	input_file(files_to_include[i]);
}

void slim_history_callback(const MxPairContraction& conx, float cost)
{
    history->add(conx);
}


static
void output_ivrml(ostream& out, bool vrml=false)
{
    uint i;

    if( vrml )
	out << "#VRML V1.0 ascii" << endl;
    else
	out << "#Inventor V2.0 ascii" << endl;

    out << "Separator {" << endl
         << "Coordinate3 {" << endl
         << "point [" << endl;

    for(i=0; i<m->vert_count(); i++)
	if( m->vertex_is_valid(i) )
	    out << "   " << m->vertex(i)[0] << " "
		<< m->vertex(i)[1] << " "
		<< m->vertex(i)[2] << "," << endl;

    out << "]"<< endl << "}" << endl;
    out << "IndexedFaceSet {" << endl
         << "coordIndex [" << endl;

    for(i=0; i<m->face_count(); i++)
	if( m->face_is_valid(i) )
	    out << "   "
		<< m->face(i)[0] << ", "
		<< m->face(i)[1] << ", "
		<< m->face(i)[2] << ", "
		<< "-1," << endl;

    out << "]}}" << endl;
}

void output_iv(ostream& out) { output_ivrml(out, false); }
void output_vrml(ostream& out) { output_ivrml(out, true); }

void output_regressive_mmf(ostream& out)
{
    if( !history ) return;

    out << "set delta_encoding 1" << endl;

    for(uint i=0; i<history->length(); i++)
    {
        MxPairContraction& conx = (*history)[i];

	// Output the basic contraction record
        out << "v% " << conx.v1+1 << " " << conx.v2+1 << " "
            << conx.dv1[X] << " " << conx.dv1[Y] << " " << conx.dv1[Z]
            << endl;

        // Output the faces that are being removed
        for(uint j=0; j<conx.dead_faces.length(); j++)
            out << "f- " << conx.dead_faces(j)+1 << endl;
    }
}

void output_regressive_log(ostream& out)
{
    if( !history ) return;

    for(uint i=0; i<history->length(); i++)
    {
        MxPairContraction& conx = (*history)[i];

	// Output the basic contraction record
        out << "v% " << conx.v1+1 << " " << conx.v2+1 << " "
            << conx.dv1[X] << " " << conx.dv1[Y] << " " << conx.dv1[Z];

        // Output the faces that are being removed
        for(uint j=0; j<conx.dead_faces.length(); j++)
            out << " " << conx.dead_faces(j)+1;

        // Output the faces that are being reshaped
        out << " &";
        for(uint k=0; k<conx.delta_faces.length(); k++)
            out << " " << conx.delta_faces(k)+1;

        out << endl;
    }
}

void output_progressive_pm(ostream& out)
{
    if( !history ) return;

    MxBlock<MxVertexID> vmap(m->vert_count());  // Maps old VIDs to new VIDs
    MxBlock<MxFaceID> fmap(m->face_count());    // Maps old FIDs to new FIDs
    uint i,k;

    MxVertexID next_vert = 0;
    MxFaceID   next_face = 0;

    ////////////////////////////////////////////////////////////////////////
    //
    // Output base mesh
    //
    for(i=0; i<m->vert_count(); i++)
	if( m->vertex_is_valid(i) )
	{
	    vmap(i) = next_vert++;
	    out << m->vertex(i) << endl;
	}
    
    for(i=0; i<m->face_count(); i++)
	if( m->face_is_valid(i) )
	{
	    fmap(i) = next_face++;
	    VID v1 = vmap(m->face(i)(0));
	    VID v2 = vmap(m->face(i)(1));
	    VID v3 = vmap(m->face(i)(2));

	    out << "f " << v1+1 << " " << v2+1 << " " << v3+1 << endl;
	}

    ////////////////////////////////////////////////////////////////////////
    //
    // Output mesh expansion
    //
    for(i=history->length()-1; i<=history->length(); i--)
    {	
	const MxPairContraction& conx = (*history)[i];
	SanityCheck( m->vertex_is_valid(conx.v1) );
	SanityCheck( !m->vertex_is_valid(conx.v2) );

	out << "v* " << vmap(conx.v1) + 1;
	out << "  "<<conx.dv1[X]<<" "<<conx.dv1[Y]<<" "<<conx.dv1[Z];
	out << "  "<<conx.dv2[X]<<" "<<conx.dv2[Y]<<" "<<conx.dv2[Z];
	out << " ";

	// Output new faces
	for(k=0; k<conx.dead_faces.length(); k++)
	{
	    FID fk = conx.dead_faces(k);
	    VID vk = m->face(fk).opposite_vertex(conx.v1, conx.v2);
 	    SanityCheck( m->vertex_is_valid(vk) );

 	    out << " ";
	    if( !m->face(fk).is_inorder(vk, conx.v1) ) out << "-";
 	    out << vmap(vk)+1;

	    fmap(conx.dead_faces(k)) = next_face++;
	    m->face_mark_valid(conx.dead_faces(k));
	}

	// Output delta faces
	out << " &";
	for(k=0; k<conx.delta_faces.length(); k++)
	{
	    out << " ";
	    FID fk = conx.delta_faces(k);
	    assert(m->face_is_valid(fk));
	    out << " " << fmap(fk)+1;
	}

	vmap(conx.v2) = next_vert++;
	m->vertex_mark_valid(conx.v2);
	out << endl;
    }
}


static ostream& vfcount(ostream& out, uint v, uint f)
{
    return out << "(" << v << "v/" << f << "f)";
}

void startup_and_input(int argc, char **argv)
{
    smf = new MxSMFReader;

    process_cmdline(argc, argv);
    if( m->face_count() == 0 )
    {
	smf->read(cin, m);
    }

    output_preamble();
}

main(int argc, char **argv)
{
    double input_time, init_time, slim_time, output_time;

    // Process command line and read input model(s)
    //
    TIMING(input_time, startup_and_input(argc, argv));

    if(!be_quiet) cerr << "+ Initial model    ";
    if(!be_quiet) vfcount(cerr, m->vert_count(), m->face_count()) << endl;

    // Initial simplification process.  Collect contractions and build heap.
    //
    TIMING(init_time, slim_init());

    // Decimate model until target is reached
    //
    TIMING(slim_time, slim->decimate(face_target));

    if(!be_quiet) cerr << "+ Simplified model ";
    if(!be_quiet) vfcount(cerr, slim->valid_verts, slim->valid_faces) << endl;

    // Output the result
    //
    TIMING(output_time, output_final_model());

    if( !be_quiet )
    {
	cerr << endl << endl;
	cerr << "+ Running time" << endl;
	cerr << "    Setup      : " << input_time << " sec" << endl;
	cerr << "    QSlim init : " << init_time << " sec" << endl;
	cerr << "    QSlim run  : " << slim_time << " sec" << endl;
	cerr << "    Output     : " << output_time << " sec" << endl;
	cerr << endl;
	cerr << "    Total      : "
	     << input_time+init_time+slim_time+output_time <<endl;
    }
    else
    {
	cerr << slim->valid_faces << " " << init_time+slim_time << endl;
    }

    slim_cleanup();

    return 0;

