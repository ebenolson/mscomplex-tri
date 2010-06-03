#include <sstream>
#include <cstring>
#include <iostream>

#include <boost/algorithm/string_regex.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/static_assert.hpp>

#include <GL/glew.h>

#include <glutils.h>
#include <GLSLProgram.h>
#include <logutil.h>

#include <grid_viewer.h>
#include <grid_datamanager.h>
#include <grid_mscomplex.h>
#include <grid_mscomplex_ensure.h>
#include <grid_dataset.h>

template<typename T> std::string to_string(const T & t)
{
  std::stringstream ss;
  ss<<t;
  return ss.str();
}

glutils::color_t g_grid_cp_colors[grid::gc_grid_dim+1] =
{
  glutils::color_t(0.0,0.0,1.0),
  glutils::color_t(0.0,1.0,0.0),
  glutils::color_t(1.0,0.0,0.0),
};

glutils::color_t g_grid_grad_colors[grid::gc_grid_dim] =
{
  glutils::color_t(0.0,0.5,0.5 ),
  glutils::color_t(0.5,0.0,0.5 ),
};

glutils::color_t g_disc_colors[grid::GRADDIR_COUNT][grid::gc_grid_dim+1] =
{
  {
    glutils::color_t(0.15,0.45,0.35 ),
    glutils::color_t(0.85,0.65,0.75 ),
    glutils::color_t(0.0,0.0,0.0 ),
  },

{
    glutils::color_t(0.0,0.0,0.0 ),
    glutils::color_t(0.65,0.95,0.45 ),
    glutils::color_t(0.15,0.25,0.75 ),
  },
};

glutils::color_t g_grid_cp_conn_colors[grid::gc_grid_dim] =
{
  glutils::color_t(0.0,0.5,0.5 ),
  glutils::color_t(0.5,0.0,0.5 ),
};

glutils::color_t g_roiaabb_color = glutils::color_t(0.85,0.75,0.65);

namespace grid
{

  glutils::vertex_t cell_to_vertex(cellid_t c)
  {

#warning "cell to vertex not implemented"
    return glutils::vertex_t(0,0,0);
  }

  glutils::vertex_t cp_to_vertex(mscomplex_t *msc,uint i)
  {

#warning "cp to vertex not implemented"
    return glutils::vertex_t(0,0,0);
  }

  grid_viewer_t::grid_viewer_t
      (data_manager_t * gdm):
      m_scale_factor(0),
      m_bRebuildRens(true),m_bShowRoiBB(false),m_bCenterToRoi(false),
      m_gdm(gdm)
  {
    for(uint i = 0 ;i < m_gdm->m_pieces.size();++i)
      m_grid_piece_rens.push_back(new octtree_piece_rendata(m_gdm->m_pieces.at(i)));

#warning "m_roi and m_roi_base_pt are not intialized"

  }

  grid_viewer_t::~grid_viewer_t()
  {
    for ( uint i = 0 ; i < m_grid_piece_rens.size();i++ )
      delete m_grid_piece_rens[i];

    m_grid_piece_rens.clear();

    delete m_gdm;
  }

  void grid_viewer_t::set_roi_dim_range_nrm(double l,double u,int dim)
  {
    if(!(l<u && 0.0 <= l && u <=1.0 && 0<=dim && dim < gc_grid_dim))
      return;

    rect_t roi  = m_extent;

    double span = roi[dim].span();

    m_roi[dim][0]  = (uint)(l*span);
    m_roi[dim][1]  = (uint)(u*span);

    m_roi_base_pt  = ((m_roi.upper_corner() +  m_roi.lower_corner())/2);
  }

  int grid_viewer_t::render()
  {
    if(m_bRebuildRens)
    {
      build_rens();

      m_bRebuildRens = false;
    }

    glPushAttrib(GL_ENABLE_BIT);

    glEnable(GL_NORMALIZE);

    glScalef(m_scale_factor,
             0.125,
             m_scale_factor);

    point_t s = m_extent.span()/2;

    if(m_bCenterToRoi)
      glTranslatef(-m_roi_base_pt[0],
                   0,
                   -m_roi_base_pt[1]);
    else
      glTranslated(-s[0],-s[1],-s[2]);

    if(m_bShowRoiBB)
    {
      glPushAttrib(GL_ENABLE_BIT);

      glDisable(GL_LIGHTING);

      glColor3dv(g_roiaabb_color.data());

      glutils::draw_aabb_line(m_roi.lower_corner(),m_roi.upper_corner());

      glPopAttrib();
    }

    for ( uint i = 0 ; i < m_grid_piece_rens.size();i++ )
    {
      m_grid_piece_rens[i]->render_msgraph_data();
    }

    for ( uint i = 0 ; i < m_grid_piece_rens.size();i++ )
    {
      m_grid_piece_rens[i]->render_dataset_data();
    }

    glPopAttrib();
  }

  void grid_viewer_t::build_rens()
  {
    for ( uint i = 0 ; i < m_grid_piece_rens.size();i++ )
    {
      m_grid_piece_rens[i]->create_cp_rens(m_roi);
      m_grid_piece_rens[i]->create_canc_cp_rens(m_roi);
      m_grid_piece_rens[i]->create_grad_rens(m_roi);
    }
  }

  void grid_viewer_t::init()
  {
    glutils::init();

    if(m_extent.eff_dim() == 0)
      throw std::runtime_error("NULL extent for viewer");

    point_t s = m_extent.span();

    m_scale_factor =0.5/ *std::max_element(s.begin(),s.end());

    /*turn back face culling off */
    glEnable ( GL_CULL_FACE );

    /*cull backface */
    glCullFace ( GL_BACK );

    /*polymode */
    glPolygonMode ( GL_FRONT, GL_FILL );

    glPolygonMode ( GL_BACK, GL_LINE );

    for ( uint i = 0 ; i < m_grid_piece_rens.size();i++ )
    {
      m_grid_piece_rens[i]->create_cp_loc_bo();
      m_grid_piece_rens[i]->create_disc_rds();
    }
  }

  int grid_viewer_t::rows()
  {
    return m_grid_piece_rens.size();
  }
  int grid_viewer_t::columns()
  {
    return 10;
  }
  bool grid_viewer_t::exchange_data(const data_index_t &idx,boost::any &v)
  {

    switch(idx[0])
    {
    case 0: return s_exchange_ro(m_grid_piece_rens[idx[1]]->dp->label(),v);
    case 1: return s_exchange_rw(m_grid_piece_rens[idx[1]]->m_bShowAllCps,v);
    case 2: return s_exchange_rw(m_grid_piece_rens[idx[1]]->m_bShowCps[0],v);
    case 3: return s_exchange_rw(m_grid_piece_rens[idx[1]]->m_bShowCps[1],v);
    case 4: return s_exchange_rw(m_grid_piece_rens[idx[1]]->m_bShowCps[2],v);
    case 5: return s_exchange_rw(m_grid_piece_rens[idx[1]]->m_bShowCpLabels,v);
    case 6: return s_exchange_rw(m_grid_piece_rens[idx[1]]->m_bShowMsGraph,v);
    case 7: return s_exchange_rw(m_grid_piece_rens[idx[1]]->m_bShowGrad,v);
    case 8: return s_exchange_rw(m_grid_piece_rens[idx[1]]->m_bShowCancCps,v);
    case 9: return s_exchange_rw(m_grid_piece_rens[idx[1]]->m_bShowCancMsGraph,v);
    }

    throw std::logic_error("unknown index");
  }
  boost::any grid_viewer_t::get_header(int i)
  {
    switch(i)
    {

    case 0: return std::string("oct tree piece");
    case 1: return std::string("all cps");
    case 2: return std::string("minima");
    case 3: return std::string("1 saddle");
    case 4: return std::string("maxima");
    case 5: return std::string("cp labels");
    case 6: return std::string("msgraph");
    case 7: return std::string("gradient");
    case 8: return std::string("cancelled cps");
    case 9: return std::string("cancelled cp msgraph");
    }

    throw std::logic_error("unknown index");
  }


  octtree_piece_rendata::octtree_piece_rendata (datapiece_t * _dp):
      m_bShowAllCps(false),
      m_bShowCpLabels ( false ),
      m_bShowMsGraph ( false ),
      m_bShowGrad ( false ),
      m_bShowCancCps(false),
      m_bShowCancMsGraph(false),
      m_bNeedUpdateDiscRens(false),
      dp(_dp)
  {
    using namespace boost::lambda;

    std::for_each(m_bShowCps,m_bShowCps+gc_grid_dim+1,_1 = false);

  }

  void octtree_piece_rendata::create_cp_loc_bo()
  {
    if(dp->msgraph == NULL)
      return;

    std::vector<glutils::vertex_t>  cp_loc;

    for(uint i = 0; i < dp->msgraph->m_cps.size(); ++i)
    {
      cp_loc.push_back(cp_to_vertex(dp->msgraph,i));
    }

    cp_loc_bo = glutils::make_buf_obj(cp_loc);
  }

  void  octtree_piece_rendata::create_cp_rens(const rect_t & roi)
  {
    if(dp->msgraph == NULL)
      return;

    std::vector<std::string>            crit_labels[gc_grid_dim+1];
    std::vector<glutils::vertex_t>      crit_label_locations[gc_grid_dim+1];
    std::vector<glutils::point_idx_t>   crit_pt_idxs[gc_grid_dim+1];
    std::vector<glutils::line_idx_t>    crit_conn_idxs[gc_grid_dim];

    for(uint i = 0; i < dp->msgraph->m_cps.size(); ++i)
    {
      if(dp->msgraph->m_cps[i]->is_paired)
        continue;

      cellid_t c = (dp->msgraph->m_cps[i]->cellid);

      if(!roi.contains(cell_to_vertex(c)))
        continue;

      uint index = dp->msgraph->m_cps[i]->index;

      std::stringstream ss;

      ((std::ostream&)ss)<<c;

      if(!dp->msgraph->m_cps[i]->is_paired)
      {
        crit_labels[index].push_back(ss.str());
        crit_label_locations[index].push_back(cp_to_vertex(dp->msgraph,i));
        crit_pt_idxs[index].push_back(i);
      }
    }

    for(uint i = 0 ; i < gc_grid_dim+1; ++i)
    {
      ren_cp_labels[i].reset(glutils::create_buffered_text_ren
                             (crit_labels[i],crit_label_locations[i]));

      ren_cp[i].reset(glutils::create_buffered_points_ren
                      (cp_loc_bo,
                       glutils::make_buf_obj(crit_pt_idxs[i]),
                       glutils::make_buf_obj()));
    }

    for(uint i = 0 ; i < dp->msgraph->m_cps.size(); ++i)
    {
      if(dp->msgraph->m_cps[i]->isCancelled)
        continue;

      if(dp->msgraph->m_cps[i]->is_paired)
        continue;

      cellid_t c = (dp->msgraph->m_cps[i]->cellid);

      if(!roi.contains(cell_to_vertex(c)))
        continue;

      uint index = dp->msgraph->m_cps[i]->index;

      for(conn_iter_t it  = dp->msgraph->m_cps[i]->conn[0].begin();
      it != dp->msgraph->m_cps[i]->conn[0].end(); ++it)
      {
        if(!roi.contains(cell_to_vertex(dp->msgraph->m_cps[*it]->cellid)))
          continue;

        crit_conn_idxs[index-1].push_back
            (glutils::line_idx_t(i,*it));
      }
    }

    for(uint i = 0 ; i < gc_grid_dim; ++i)
    {
      ren_cp_conns[i].reset(glutils::create_buffered_lines_ren
                            (cp_loc_bo,
                             glutils::make_buf_obj(crit_conn_idxs[i]),
                             glutils::make_buf_obj()));
    }

  }

  void  octtree_piece_rendata::create_canc_cp_rens(const rect_t & roi)
  {
    if(dp->msgraph == NULL)
      return;

    std::vector<std::string>            canc_cp_labels[gc_grid_dim+1];
    std::vector<glutils::vertex_t>      canc_cp_label_locations[gc_grid_dim+1];
    std::vector<glutils::point_idx_t>   canc_cp_idxs[gc_grid_dim+1];
    std::vector<glutils::line_idx_t>    canc_cp_conn_idxs[gc_grid_dim];

    for(uint i = 0; i < dp->msgraph->m_cps.size(); ++i)
    {
      if(!dp->msgraph->m_cps[i]->is_paired)
        continue;

      cellid_t c = (dp->msgraph->m_cps[i]->cellid);

      if(!roi.contains(cell_to_vertex(c)))
        continue;

      uint index = dp->msgraph->m_cps[i]->index;

      canc_cp_labels[index].push_back(to_string(i));
      canc_cp_label_locations[index].push_back(cp_to_vertex(dp->msgraph,i)) ;
      canc_cp_idxs[index].push_back(i);

    }

    for(uint i = 0 ; i < gc_grid_dim+1; ++i)
    {

      ren_canc_cp_labels[i].reset(glutils::create_buffered_text_ren
                                  (canc_cp_labels[i],canc_cp_label_locations[i]));

      ren_canc_cp[i].reset(glutils::create_buffered_points_ren
                           (cp_loc_bo,
                            glutils::make_buf_obj(canc_cp_idxs[i]),
                            glutils::make_buf_obj()));
    }

    for(uint i = 0 ; i < dp->msgraph->m_cps.size(); ++i)
    {
      if(!dp->msgraph->m_cps[i]->is_paired)
        continue;

      cellid_t c = (dp->msgraph->m_cps[i]->cellid);

      if(!roi.contains(cell_to_vertex(c)))
        continue;

      uint index = dp->msgraph->m_cps[i]->index;

      for(uint dir = 0 ; dir <2 ;++dir)
      {
        for(conn_iter_t it  = dp->msgraph->m_cps[i]->conn[dir].begin();
        it != dp->msgraph->m_cps[i]->conn[dir].end(); ++it)
        {
          if(!roi.contains(cell_to_vertex(dp->msgraph->m_cps[*it]->cellid)))
            continue;

          canc_cp_conn_idxs[index-(dir^1)].push_back
              (glutils::line_idx_t(i,*it));
        }
      }
    }

    for(uint i = 0 ; i < gc_grid_dim; ++i)
    {
      ren_canc_cp_conns[i].reset(glutils::create_buffered_lines_ren
                                 (cp_loc_bo,
                                  glutils::make_buf_obj(canc_cp_conn_idxs[i]),
                                  glutils::make_buf_obj()));
    }

  }

  void octtree_piece_rendata::create_grad_rens(const rect_t & roi)
  {
#warning "create_grad_rens not implemented"
  }


  void octtree_piece_rendata::create_disc_rds()
  {
    if(dp->msgraph == NULL)
      return;

    boost::shared_ptr<disc_rendata_t> sptr;

    for(uint i = 0 ; i < dp->msgraph->m_cps.size();++i)
    {
      critpt_t * cp = dp->msgraph->m_cps[i];

      if(cp->is_paired) continue;

      sptr.reset(new disc_rendata_t(cp->cellid,cp->index));

      disc_rds.push_back(sptr);
    }
  }

  void octtree_piece_rendata::update_active_disc_rens()
  {
    if(dp->msgraph == NULL)
      return;

    for(uint i = 0 ; i < disc_rds.size();++i)
    {
      if(disc_rds[i]->update(dp->msgraph))
      {
        active_disc_rens.insert(disc_rds[i]);
      }
      else
      {
        active_disc_rens.erase(disc_rds[i]);
      }
    }
  }

  void octtree_piece_rendata::render_msgraph_data()
  {
    glPushMatrix();
    glPushAttrib ( GL_ENABLE_BIT );

    glDisable ( GL_LIGHTING );

    glPointSize ( 4.0 );

    for(uint i = 0 ; i < gc_grid_dim+1;++i)
    {
      if(ren_cp[i]&& (m_bShowCps[i]||m_bShowAllCps))
      {
        glColor3dv(g_grid_cp_colors[i].data());

        ren_cp[i]->render();

        if(ren_cp_labels[i] && m_bShowCpLabels)
          ren_cp_labels[i]->render();
      }
    }


    if ( m_bShowCancCps)
    {
      for(uint i = 0 ; i < gc_grid_dim+1;++i)
      {
        if(ren_canc_cp[i])
        {
          glColor3dv(g_grid_cp_colors[i].data());

          ren_canc_cp[i]->render();

          if(ren_canc_cp_labels[i] &&
             m_bShowCpLabels)
            ren_canc_cp_labels[i]->render();
        }
      }
    }

    if (m_bShowMsGraph)
    {
      for(uint i = 0 ; i < gc_grid_dim;++i)
      {
        if(ren_cp_conns[i])
        {
          glColor3dv(g_grid_cp_conn_colors[i].data());

          ren_cp_conns[i]->render();
        }
      }
    }

    if (m_bShowCancMsGraph)
    {
      for(uint i = 0 ; i < gc_grid_dim;++i)
      {
        if(ren_canc_cp_conns[i])
        {
          glColor3dv(g_grid_cp_conn_colors[i].data());

          ren_canc_cp_conns[i]->render();
        }
      }
    }

    glPopAttrib();
    glPopMatrix();
  }

  void octtree_piece_rendata::render_dataset_data()
  {
    if(m_bNeedUpdateDiscRens)
    {
      update_active_disc_rens();
      m_bNeedUpdateDiscRens = false;
    }

    glPushMatrix();
    glPushAttrib ( GL_ENABLE_BIT );

    glDisable ( GL_LIGHTING );

    if(m_bShowGrad)
    {
      for(uint i = 0 ; i < gc_grid_dim; ++i)
      {
        if(ren_grad[i])
        {
          glColor3dv ( g_grid_grad_colors[i].data() );

          ren_grad[i]->render();
        }
      }
    }


    for(disc_rendata_sp_set_t::iterator it = active_disc_rens.begin();
        it != active_disc_rens.end() ; ++it)
    {
      (*it)->render();
    }

    glPopAttrib();
    glPopMatrix();

  }

  int octtree_piece_rendata::rows()
  {
    return disc_rds.size();
  }
  int octtree_piece_rendata::columns()
  {
    return 5;
  }
  bool octtree_piece_rendata::exchange_data
      (const data_index_t &idx,boost::any &v)
  {
    bool need_update = false;

    bool is_read     = v.empty();

    int i = idx[0];

    switch(i)
    {
    case 0:
      return s_exchange_ro((int)disc_rds[idx[1]]->index,v);
    case 1:
    case 2:
      need_update =  s_exchange_rw(disc_rds[idx[1]]->show[i%2],v);break;
    case 3:
    case 4:
      return s_exchange_rw(disc_rds[idx[1]]->color[i%2],v);
    };

    if(need_update && is_read == false )
      m_bNeedUpdateDiscRens = true;

    return need_update;

  }

  boost::any octtree_piece_rendata::get_header(int i)
  {
    switch(i)
    {
    case 0: return std::string("index");
    case 1: return std::string("asc disc");
    case 2: return std::string("des disc");
    case 3: return std::string("asc disc color");
    case 4: return std::string("des disc color");
    }
    throw std::logic_error("invalid index");
  }

  disc_rendata_t::disc_rendata_t(cellid_t c,uint i):cellid(c),index(i)
  {
    color[0] = g_disc_colors[1][index];
    color[1] = g_disc_colors[0][index];

    show[0] =false; ren[0] =NULL;
    show[1] =false; ren[1] =NULL;
  }

  disc_rendata_t::~disc_rendata_t()
  {
    show[0] =false;
    show[1] =false;

    update(NULL);

  }

  void disc_rendata_t::render()
  {
    for(uint dir = 0 ; dir<2;++dir)
    {
      if(show[dir])
      {
        glColor3dv(g_grid_cp_colors[index].data());

        glBegin(GL_POINTS);
        glVertex3dv(cell_to_vertex(cellid).data());
        glEnd();

        glColor3dv(color[dir].data());

        ren[dir]->render();
      }
    }
  }

  bool disc_rendata_t::update(mscomplex_t *msc)
  {

    using namespace boost::lambda;

    for(uint dir = 0 ; dir<2;++dir)
    {
      if(show[dir] && this->ren[dir] == NULL && msc)
      {
        ensure_cellid_critical(msc,cellid);

        critpt_t *cp = msc->m_cps[msc->m_id_cp_map[cellid]];

        std::set<cellid_t> vset;

        for(uint j = 0 ; j < cp->contrib[dir].size();++j)
        {
          critpt_t *cp_contrib = msc->m_cps[cp->contrib[dir][j]];

          if(cp_contrib->index != cp->index)
            throw std::logic_error("contrib and cp must have same idx");

          for(uint i = 0; i < cp_contrib->disc[dir].size(); ++i)
          {
            cellid_t c = cp_contrib->disc[dir][i];

            if(vset.count(c) == 0)
              vset.insert(c);
          }
        }

        std::vector<glutils::vertex_t> vlist(vset.size());

        std::transform(vset.begin(),vset.end(),vlist.begin(),cell_to_vertex);

        ren[dir] = glutils::create_buffered_points_ren
                   (glutils::make_buf_obj(vlist),
                    glutils::make_buf_obj(),
                    glutils::make_buf_obj());

      }

      if(!show[dir] && this->ren[dir] != NULL )
      {
        delete ren[dir];

        ren[dir] = NULL;

      }
    }

    return (show[0] || show[1]);
  }

}
