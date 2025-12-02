// ChildView.cpp
#include "pch.h"
#include "framework.h"
#include "MFCApplication5.h" // (주의) 본인 프로젝트 이름 헤더가 있다면 포함
#include "ChildView.h"

#include <cmath>       
#include <wchar.h>   
#include <Shlwapi.h>   

// 라이브러리 링크 (Shlwapi.lib 필요)
#pragma comment(lib, "Shlwapi.lib")

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CChildView::CChildView()
{
    m_edgeStartIndex = -1;
    m_pathStartIndex = -1;
    m_pathEndIndex = -1;
    m_bMapLoaded = FALSE;

    // ===== 지도 이미지 로드 =====
    WCHAR exePath[MAX_PATH];
    if (GetModuleFileNameW(nullptr, exePath, MAX_PATH))
    {
        PathRemoveFileSpecW(exePath);
        wcscat_s(exePath, L"\\map.jpg"); // map.jpg 로드

        HRESULT hr = m_map.Load(exePath);
        if (SUCCEEDED(hr))
        {
            m_bMapLoaded = TRUE;
        }
        else
        {
            // 로드 실패 시 디버깅용 메시지 (필요 없으면 주석 처리)
            // AfxMessageBox(L"이미지 로드 실패. exe 폴더에 map.jpg를 확인하세요.");
        }
    }
}

CChildView::~CChildView()
{
}

BEGIN_MESSAGE_MAP(CChildView, CWnd)
    ON_WM_PAINT()
    ON_WM_ERASEBKGND()
    ON_WM_LBUTTONDOWN()
END_MESSAGE_MAP()

BOOL CChildView::PreCreateWindow(CREATESTRUCT& cs)
{
    if (!CWnd::PreCreateWindow(cs))
        return FALSE;

    cs.dwExStyle |= WS_EX_CLIENTEDGE;
    cs.style &= ~WS_BORDER;
    cs.lpszClass = AfxRegisterWndClass(
        CS_HREDRAW | CS_VREDRAW,
        ::LoadCursor(nullptr, IDC_ARROW),
        reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1),
        nullptr);

    return TRUE;
}

BOOL CChildView::OnEraseBkgnd(CDC* pDC)
{
    // 깜빡임 방지를 위해 배경 지우기 생략 (OnPaint에서 다 그림)
    return TRUE;
}

int CChildView::HitTestNode(CPoint pt)
{
    const int r = 15; // 클릭 판정 범위 (좀 넉넉하게 15로 설정)

    // 뒤에서부터 확인하면 마지막에 만든 노드를 우선 선택
    for (int i = (int)m_nodes.size() - 1; i >= 0; --i)
    {
        CRect rc(m_nodes[i].pt.x - r, m_nodes[i].pt.y - r,
            m_nodes[i].pt.x + r, m_nodes[i].pt.y + r);
        if (rc.PtInRect(pt))
            return i;
    }
    return -1;
}

void CChildView::DrawGraph(CDC* pDC)
{
    // 1. 엣지 그리기
    for (const auto& e : m_edges)
    {
        if (e.u < 0 || e.v < 0 || e.u >= (int)m_nodes.size() || e.v >= (int)m_nodes.size())
            continue;

        CPoint p1 = m_nodes[e.u].pt;
        CPoint p2 = m_nodes[e.v].pt;

        // 최단 경로는 빨간색 굵은 선, 일반 경로는 검은색 얇은 선
        int penWidth = e.isShortest ? 5 : 2;
        COLORREF penColor = e.isShortest ? RGB(255, 0, 0) : RGB(0, 0, 0);

        CPen pen(PS_SOLID, penWidth, penColor);
        CPen* pOldPen = pDC->SelectObject(&pen);

        pDC->MoveTo(p1);
        pDC->LineTo(p2);

        pDC->SelectObject(pOldPen);
    }

    // 2. 노드 그리기
    const int r = 6;
    for (size_t i = 0; i < m_nodes.size(); ++i)
    {
        const NODE_INFO& n = m_nodes[i];

        // 선택된 노드는 파란색, 일반 노드는 하늘색
        COLORREF color = n.selected ? RGB(0, 0, 255) : RGB(0, 128, 255);
        CBrush brush(color);
        CBrush* pOldBrush = pDC->SelectObject(&brush);

        pDC->Ellipse(n.pt.x - r, n.pt.y - r, n.pt.x + r, n.pt.y + r);

        pDC->SelectObject(pOldBrush);
    }
}

void CChildView::OnPaint()
{
    CPaintDC dc(this);

    // 더블 버퍼링 (깜빡임 제거용)
    CDC memDC;
    CBitmap bitmap;
    CRect rcClient;
    GetClientRect(&rcClient);

    memDC.CreateCompatibleDC(&dc);
    bitmap.CreateCompatibleBitmap(&dc, rcClient.Width(), rcClient.Height());
    CBitmap* pOldBitmap = memDC.SelectObject(&bitmap);

    // 1. 배경(지도) 그리기
    if (m_bMapLoaded && !m_map.IsNull())
    {
        m_map.Draw(memDC.m_hDC, 0, 0, m_map.GetWidth(), m_map.GetHeight());
    }
    else
    {
        memDC.FillSolidRect(&rcClient, RGB(255, 255, 255));
    }

    // 2. 그래프(점과 선) 그리기
    DrawGraph(&memDC);

    // 3. 화면에 복사
    dc.BitBlt(0, 0, rcClient.Width(), rcClient.Height(), &memDC, 0, 0, SRCCOPY);

    memDC.SelectObject(pOldBitmap);
}

void CChildView::OnLButtonDown(UINT nFlags, CPoint point)
{
    BOOL bCtrl = (nFlags & MK_CONTROL) != 0;
    BOOL bShift = (nFlags & MK_SHIFT) != 0;
    // Alt키 확인 (GetKeyState 사용)
    BOOL bAlt = (GetKeyState(VK_MENU) < 0);

    // 1) Ctrl + 클릭 : 노드 생성
    if (bCtrl && !bAlt && !bShift)
    {
        m_nodes.push_back(NODE_INFO(point));
        Invalidate();
    }
    // 2) Alt + 클릭 : 두 노드 연결 (엣지 생성)
    else if (bAlt && !bCtrl && !bShift)
    {
        int idx = HitTestNode(point);
        if (idx != -1)
        {
            if (m_edgeStartIndex == -1)
            {
                // 첫 번째 노드 선택
                m_edgeStartIndex = idx;
                m_nodes[idx].selected = true;
            }
            else if (m_edgeStartIndex != idx)
            {
                // 두 번째 노드 선택 -> 엣지 추가
                int u = m_edgeStartIndex;
                int v = idx;

                double dist = sqrt(pow(m_nodes[u].pt.x - m_nodes[v].pt.x, 2) +
                    pow(m_nodes[u].pt.y - m_nodes[v].pt.y, 2));

                m_edges.push_back(EDGE_INFO(u, v, dist));

                // 선택 상태 해제
                m_nodes[m_edgeStartIndex].selected = false;
                m_nodes[v].selected = false;
                m_edgeStartIndex = -1;
            }
            Invalidate();
        }
    }
    // 3) Shift + 클릭 : 최단 경로 탐색
    else if (bShift && !bCtrl && !bAlt)
    {
        int idx = HitTestNode(point);
        if (idx != -1)
        {
            if (m_pathStartIndex == -1)
            {
                m_pathStartIndex = idx;
                m_nodes[idx].selected = true;
                AfxMessageBox(L"출발점이 선택되었습니다. 도착점을 Shift+클릭하세요.");
            }
            else if (m_pathEndIndex == -1 && idx != m_pathStartIndex)
            {
                m_pathEndIndex = idx;
                m_nodes[idx].selected = true;

                // Dijkstra 실행 
                RunDijkstra(m_pathStartIndex, m_pathEndIndex);

                // 초기화 (선택 표시 해제)
                m_nodes[m_pathStartIndex].selected = false;
                m_nodes[m_pathEndIndex].selected = false;
                m_pathStartIndex = -1;
                m_pathEndIndex = -1;
            }
            Invalidate();
        }
    }

    CWnd::OnLButtonDown(nFlags, point);
}

void CChildView::RunDijkstra(int start, int goal)
{
    int n = (int)m_nodes.size();
    if (start < 0 || goal < 0 || start >= n || goal >= n) return;

    const double INF = 1e18;
    std::vector<double> dist(n, INF);
    std::vector<int> prev(n, -1);
    std::vector<bool> visited(n, false);

    dist[start] = 0.0;

    // 기존 최단 경로 표시 초기화
    for (auto& e : m_edges) e.isShortest = false;

    // 다익스트라 루프
    for (int i = 0; i < n; ++i)
    {
        int u = -1;
        double best = INF;

        // 방문하지 않은 노드 중 최소 거리 노드 선택
        for (int v = 0; v < n; ++v)
        {
            if (!visited[v] && dist[v] < best)
            {
                best = dist[v];
                u = v;
            }
        }

        if (u == -1 || dist[u] == INF) break;
        visited[u] = true;
        if (u == goal) break;

        // 인접 노드 갱신
        for (const auto& e : m_edges)
        {
            int v = -1;
            if (e.u == u) v = e.v;
            else if (e.v == u) v = e.u; // 양방향

            if (v != -1)
            {
                if (dist[u] + e.weight < dist[v])
                {
                    dist[v] = dist[u] + e.weight;
                    prev[v] = u;
                }
            }
        }
    }

    if (dist[goal] == INF)
    {
        AfxMessageBox(L"경로가 존재하지 않습니다.");
        return;
    }

    // 경로 역추적하여 엣지 강조
    int cur = goal;
    while (prev[cur] != -1)
    {
        int p = prev[cur];
        for (auto& e : m_edges)
        {
            // p와 cur를 연결하는 엣지 찾기
            if ((e.u == p && e.v == cur) || (e.u == cur && e.v == p))
            {
                e.isShortest = true;
                break;
            }
        }
        cur = p;
    }

    CString msg;
    msg.Format(L"최단 거리: %.1f 픽셀", dist[goal]);
    AfxMessageBox(msg);
}