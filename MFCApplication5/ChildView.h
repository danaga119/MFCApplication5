// ChildView.h
#pragma once
#include <vector>
#include <atlimage.h>

// 1. 구조체 정의 (cpp 파일에서 사용하는 이름과 통일)
struct NODE_INFO {
    CPoint pt;
    bool selected;
    // 생성자
    NODE_INFO(CPoint p) : pt(p), selected(false) {}
};

struct EDGE_INFO {
    int u, v;       // 시작점, 끝점 인덱스
    double weight;  // 거리
    bool isShortest;// 최단 경로 포함 여부
    // 생성자
    EDGE_INFO(int _u, int _v, double _w)
        : u(_u), v(_v), weight(_w), isShortest(false) {
    }
};

// CChildView 클래스
class CChildView : public CWnd
{
public:
    CChildView();
    virtual ~CChildView();

protected:
    virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

    // 멤버 변수들
public:
    CImage m_map;                   // 지도 이미지 객체
    BOOL m_bMapLoaded;              // 지도 로드 성공 여부

    std::vector<NODE_INFO> m_nodes; // 노드 리스트
    std::vector<EDGE_INFO> m_edges; // 엣지 리스트

    int m_edgeStartIndex;           // 엣지 연결 시 첫 번째 선택한 노드
    int m_pathStartIndex;           // 경로 찾기 시작점
    int m_pathEndIndex;             // 경로 찾기 도착점

    // 멤버 함수들
public:
    int HitTestNode(CPoint pt);            // 클릭한 곳에 노드가 있는지 확인
    void DrawGraph(CDC* pDC);              // 그래프 그리기
    void RunDijkstra(int start, int goal); // 다익스트라 알고리즘

protected:
    afx_msg void OnPaint();
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    DECLARE_MESSAGE_MAP()
};