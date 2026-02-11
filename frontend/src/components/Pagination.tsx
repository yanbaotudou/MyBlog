interface PaginationProps {
  page: number;
  pageSize: number;
  total: number;
  onPageChange: (next: number) => void;
}

export function Pagination({ page, pageSize, total, onPageChange }: PaginationProps) {
  const totalPages = Math.max(1, Math.ceil(total / pageSize));

  return (
    <div className="pagination">
      <button disabled={page <= 1} onClick={() => onPageChange(page - 1)}>
        上一页
      </button>
      <span>
        第 {page} / {totalPages} 页，共 {total} 条
      </span>
      <button disabled={page >= totalPages} onClick={() => onPageChange(page + 1)}>
        下一页
      </button>
    </div>
  );
}
