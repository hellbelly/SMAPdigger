#include<iostream>
#include<algorithm>
#include<string.h>
#include "dbseq.h"

using namespace std;

extern Param param;
extern bit8_t alphabet[];
extern bit8_t rev_alphabet[];

/************/
RefSeq::RefSeq()
{
	total_kmers=0;
}

ref_loc_t RefSeq::LoadNextSeq(ifstream &fin) {
    char ch[1000]; char c; string s;
    if(param.gz_ref) return 0;
    fin>>c; if(fin.eof()) return 0;
    string::iterator z=_seq.begin();
    _length=0;
    fin>>_name; fin.getline(ch, 1000);
    while(!fin.eof()) {
        fin>>c; if(fin.eof()) break;
        fin.unget(); if(c=='>') break;
        fin>>s;
        if(_length+s.size()>=param.max_dbseq_size) {
            param.max_dbseq_size+=param.append_dbseq_size;
            _seq.resize(param.max_dbseq_size);
            z=_seq.begin()+_length;
            //cout<<"_seq size: "<<param.max_dbseq_size<<endl;
        }
        copy(s.begin(), s.end(), z);
        z+=s.size(); _length+=s.size();
    }
    return _length;
}

ref_loc_t RefSeq::LoadNextSeq(igzstream &fin) {
	char ch[1000]; char c; string s;
	if(!param.gz_ref) return 0;
	fin>>c; if(fin.eof()) return 0;
	string::iterator z=_seq.begin();
	_length=0; 
	fin>>_name; fin.getline(ch, 1000);
	while(!fin.eof()) {
		fin>>c; if(fin.eof()) break;
		fin.unget(); if(c=='>') break;
		fin>>s;
		if(_length+s.size()>=param.max_dbseq_size) {
			param.max_dbseq_size+=param.append_dbseq_size;
			_seq.resize(param.max_dbseq_size);
			z=_seq.begin()+_length;
			//cout<<"_seq size: "<<param.max_dbseq_size<<endl;
		}
		copy(s.begin(), s.end(), z);
		z+=s.size(); _length+=s.size();
	}
	return _length;
}

void RefSeq::BinSeq(OneBfa &a) {
	a.n=(_length+(SEGLEN-1))/SEGLEN+BINSEQPAD;   //SEGLENbp, bit(SEGLEN*2) for each element. put 2 extra elements at the 3'end to invoid overflow
	bit32_t t=a.n*SEGLEN-_length;
	if(t) {
		string ts(t, 'N');
		if(_seq.size()<_length+t) _seq.resize(_length+t);
		copy(ts.begin(), ts.end(), _seq.begin()+_length);
	}
	a.s = new bit32_t[a.n];
	string::iterator p=_seq.begin();
	bit32_t i,j;
	for(i=0; i<a.n; i++,p+=SEGLEN) {
		a.s[i]=0;
		for(j=0; j<SEGLEN; j++) {
			a.s[i]<<=2;
			a.s[i]|=alphabet[(unsigned char)*(p+j)];
			//cout << *(p+j);
		}
	}
	//cout <<endl;
}

void RefSeq::cBinSeq(OneBfa &a) {
        a.n=(_length+(SEGLEN-1))/SEGLEN+BINSEQPAD;   //SEGLENbp, bit(SEGLEN*2) for each element. put 2 extra elements at the 3'end to invoid overflow
/*	int t=a.n*SEGLEN-_length;
	cout << _length<< " "<<a.n << " t "<<t<<endl; 
	if(t) {
		string ts(t, 'N');
		if(_seq.size()<_length+t) _seq.resize(_length+t);
		copy(ts.begin(), ts.end(), _seq.begin()+_length);
	}
*/
	a.s = new bit32_t[a.n];
	string::iterator p=_seq.begin()+a.n*SEGLEN-1;
	string::iterator tmp;
	for(bit32_t i=0;i<a.n; i++,p-=SEGLEN) {
		a.s[i]=0;
		for(bit32_t j=0; j<SEGLEN; j++) {
			a.s[i]<<=2;
			a.s[i]|=rev_alphabet[(unsigned char)*(p-j)];
			//cout <<*(p-j);
		}
		//cout << endl;
		//cout << StrSeed2((unsigned int)a.s[i].a, SEGLEN);
	}
	//cout <<endl;
}
                                                                                                                                                                                                                                                        

void RefSeq::UnmaskRegion() {
	Block b, cb;
	b.id=_count;
	cb.id=_count+1;
	int total_len=((_length+(SEGLEN-1))/SEGLEN+BINSEQPAD)*SEGLEN;
	b.begin=b.end=0;
//	bit32_t total_size=0;
	while(b.end<_length) {
		b.begin=_seq.find_first_of(param.useful_nt, b.end);
		if(b.begin > _length) break;
		b.end=_seq.find_first_of(param.nx_nt, b.begin);
		b.end = (b.end<=_length? b.end : _length);
		if(b.end-b.begin <30) continue;
		if((!_blocks.empty()) && (b.id==_blocks[_blocks.size()-1].id) 
			&& (b.begin - _blocks[_blocks.size()-1].end <5))
			_blocks[_blocks.size()-1].end=b.end;
		else {
			_blocks.push_back(b);
			//added by yxi
			cb.begin=total_len-b.end;
			cb.end=total_len-b.begin;
			_blocks.push_back(cb);
			//cout <<"bid "<<b.id<<" "<<b.begin<<" "<<b.end<<endl;
			//cout <<"cbid "<<cb.id<<" "<<cb.begin<<" "<<cb.end<<endl;
		}
	}
	
}

void RefSeq::find_CCGG() {
    bit32_t i,tmp_offset,tmp_max, seedloc, right=0xffffffffu;
    vector<ref_loc_t> tmpset_index_BSW[50], tmpset_index_BSC[50], tmp_CCGG_sites;
    vector<ref_loc_t>::iterator rit;
    
    std::transform(_seq.begin(),_seq.end(),_seq.begin(),static_cast < int(*)(int) > (toupper));

    tmp_offset=title[_count-1].rc_offset-param.seed_size;
    tmp_max=title[_count-1].size-param.seed_size;
    right=_seq.find(param.digest_site,right+1);
    while(right<_length) {
        //tmplen=right-left+param.digest_site.size()-param.digest_pos*2;
        n_CCGG++;
        //cout<<" left:"<<left<<" right:"<<right<<endl;

        tmp_CCGG_sites.push_back(right+param.digest_pos);
        right=_seq.find(param.digest_site,right+1);
    }
    
    CCGG_sites.push_back(tmp_CCGG_sites);
    //cout<<tmp_CCGG_sites.size()<<endl;; 

    for(i=0;i<param.max_seedseg_num;i++) {
        tmpset_index_BSW[i].reserve(tmp_CCGG_sites.size());
        tmpset_index_BSC[i].reserve(tmp_CCGG_sites.size());
    }

    for(rit=tmp_CCGG_sites.begin();rit!=tmp_CCGG_sites.end()-1&&tmp_CCGG_sites.size()>1;++rit) {
        if(*(rit+1)-*rit<=param.max_insert) {
            for(i=0,seedloc=*rit;i<param.max_seedseg_num&&seedloc<=tmp_max;i++,seedloc+=param.seed_size)
                tmpset_index_BSW[i].push_back(seedloc);
        }
    }
    
    for(rit=tmp_CCGG_sites.begin()+1;rit!=tmp_CCGG_sites.end()&&tmp_CCGG_sites.size()>1;++rit) {
        if(*(rit)-*(rit-1)<=param.max_insert) {
            for(i=0,seedloc=*rit+param.digest_site.size()-2*param.digest_pos-param.seed_size;i<param.max_seedseg_num&&seedloc>=0;i++,seedloc-=param.seed_size)
                tmpset_index_BSC[i].push_back(tmp_offset-seedloc);
        }
    }

	//next=_seq.find(param.digest_site,right+1);
	//tmplen=next-right+param.digest_site.size()-param.digest_pos*2;
	//if((tmplen<=param.max_insert)&&(next>=0))
    //    for(i=0,seedloc=right+param.digest_pos;i<param.max_seedseg_num&&seedloc<=tmp_max;i++,seedloc+=param.seed_size)
    //        tmpset_index_BSW[i].push_back(seedloc);

	//next=_seq.rfind(param.digest_site,right-1);
	//tmplen=right-left+param.digest_site.size()-param.digest_pos*2;
	//if((tmplen<=param.max_insert)&&(left>=0))
    //    for(i=0,seedloc=right+param.digest_site.size()-param.digest_pos-param.seed_size;i<param.max_seedseg_num&&seedloc>=0;i++,seedloc-=param.seed_size)
    //        tmpset_index_BSC[i].push_back(tmp_offset-seedloc);

//        left=right;
//    }
/*
    if(left>0){
        ccgg_seglen[tmploc]=_length-left-1;
    }
*/
    for(i=0;i<param.max_seedseg_num;i++) {
        CCGG_index[i].push_back(tmpset_index_BSW[i]); 
        CCGG_index[i].push_back(tmpset_index_BSC[i]); 
    }
    //CCGG_sites.push_back(tmpset_index_BSW[0]);

}

bool BlockComp(Block a, Block b) {return (a.id<b.id)||((a.id==b.id)&&(a.begin<b.begin));}

void RefSeq::Run_ConvertBinseq(ifstream &fin, igzstream &gzfin) {
    param.max_seedseg_num=(FIXELEMENT-1)*SEGLEN/param.seed_size; 
	_seq.resize(param.max_dbseq_size);
	RefTitle r;
	_count=0;
	total_num=sum_length=0;
	while(LoadNextSeq(fin)||LoadNextSeq(gzfin)) {
		r.name=_name;
		r.size=_length;
		r.rc_offset=((_length+(SEGLEN-1))/SEGLEN+BINSEQPAD)*SEGLEN;
		title.push_back(r);
		
		OneBfa a;
		BinSeq(a);
		bfa.push_back(a);
		UnmaskRegion();
		_count++;
		total_num++;
		sum_length+=_length;
//		cout<<r.size<<endl;
		
		//added by yxi, RC reference seq
		title.push_back(r);
		OneBfa ca;
		cBinSeq(ca);
        bfa.push_back(ca);
		//UnmaskRegion();
		_count++;   
		//total_num++;
		//sum_length+=_length;                                                                 		
	    //cout<<"ref:"<<r.name<<endl;
	    if(param.RRBS_flag) find_CCGG();
	}
    sort(_blocks.begin(),_blocks.end(),BlockComp);
	param.total_ref_seq=total_num;

    bit32_t i, s=0, *ptr, *cptr;
    ref_anchor.push_back(REF_MARGIN*SEGLEN);
    for(i=0;i<total_num;i++) {
        s+=bfa[i*2].n; ref_anchor.push_back((s+REF_MARGIN)*SEGLEN);
    }

    //for(i=0;i<=total_num;i++) cout<<"i="<<i<<" "<<ref_anchor[i]<<" "<<endl;

    refcat=new bit32_t[s+REF_MARGIN*2]; crefcat=new bit32_t[s+REF_MARGIN*2];
    xref[0]=refcat; xref[1]=crefcat;
    ptr=refcat+REF_MARGIN; cptr=crefcat+REF_MARGIN;
    for(i=0;i<_count;i++) {
        if(i%2==0) {
            ptr=copy(bfa[i].s, bfa[i].s+bfa[i].n, ptr);
            delete [] bfa[i].s;
            bfa[i].s=ptr-bfa[i].n;
        }
        else {
            cptr=copy(bfa[i].s, bfa[i].s+bfa[i].n, cptr);
            delete [] bfa[i].s;
            bfa[i].s=cptr-bfa[i].n;            
        }
    }


//	cout<<"total seq length: "<<sum_length<<endl;
//    cout<<"total "<<ccgg_seglen.size()<<" CCGG sites\n";
//    for(map<shortHit,bit32_t,shorthitcompclass>::iterator it=ccgg_seglen.begin();it!=ccgg_seglen.end();it++)
//    cout<<"chr:"<<(int)(it->first).chr<<" pos:"<<(it->first).loc<<" => "<<(it->second)<<endl;

	_seq.clear(); //free ram
}
                                                
//continues seed

inline bit32_t RefSeq::s_MakeSeed_1(bit32_t *_m, int _a) {
	//return param.XT(((*_m<<_a)|((bit64_t)*(_m+1))>>(SEGLEN*2-_a))>>(SEGLEN*2-param.seed_size*2)&param.seed_bits);
    return param.XT(((((bit64_t)*_m<<32)|*(_m+1))>>_a)&param.seed_bits);
   	//return param.XT(((bit64_t)*_m>>(64-_a-param.seed_size*2))&param.seed_bits);
}


inline bit32_t RefSeq::s_MakeSeed_2(bit32_t *_m, int _a) {
	//return param.XT(((*_m<<_a)|((bit64_t)*(_m+1))>>(SEGLEN*2-_a))>>(SEGLEN*2-param.seed_size*2)&param.seed_bits);
    return ((((bit64_t)*_m<<32)|*(_m+1))>>_a)&param.seed_bits;
   	//return param.XT(((bit64_t)*_m>>(64-_a-param.seed_size*2))&param.seed_bits);
}

/*
inline bit32_t RefSeq::s_MakeSeed_1(bit24_t *_m, int _a)
{
	return param.XT(((*_m<<_a)|((bit64_t)*(_m+1))>>(SEGLEN*2-_a))>>(SEGLEN*2-param.seed_size*2)&param.seed_bits);
}
*/

void RefSeq::InitialIndex() {
	bit32_t i; total_kmers=1;
	KmerLoc *p; KmerLoc2 *p2;

	for(i=0;i<param.seed_size;i++) total_kmers*=3;
	cerr<<"total_kmers: "<<total_kmers<<endl;
	if(param.RRBS_flag) {
    	index= new KmerLoc[total_kmers];
    	for(i=0,p=index; i<total_kmers; p++,i++) p->n1=0;
	}
	else {
        index2=new KmerLoc2[total_kmers];
        for(i=0,p2=index2;i<total_kmers;i++,p2++) p2->n[0]=p2->n[1]=0;
	}
}

void RefSeq::CalKmerFreq() {
	bit32_t *_m, of=64-param.seed_size*2, chr, chr1, tmp_offset, j;
	vector<ref_loc_t>::iterator it;
	if(param.RRBS_flag){
        for(j=0;j<param.max_seedseg_num;j++){    
            for(chr=0;chr<_count;chr++){
                _m=bfa[chr].s;
                for(it=CCGG_index[j][chr].begin();it!=CCGG_index[j][chr].end();++it){
                    index[s_MakeSeed_1(_m+(*it)/SEGLEN,of-((*it)%SEGLEN)*2)].n1++;
                }
                if(param.pairend||param.chains){
                    chr1=chr^1;  tmp_offset=title[chr].rc_offset-param.seed_size;
                    for(it=CCGG_index[j][chr1].begin();it!=CCGG_index[j][chr1].end();++it){
                        index[s_MakeSeed_1(_m+(tmp_offset-*it)/SEGLEN,of-((tmp_offset-*it)%SEGLEN)*2)].n1++;
                    }
                }
            }
        }
	}
	else{
		t_CalKmerFreq(0);
		t_CalKmerFreq(1);
    }
}

void RefSeq::t_CalKmerFreq(bit32_t ref_chain) {
	bit32_t *_m, i,i2,of=64-param.seed_size*2, prefetch=PREFETCH_CAL_INDEX/4*param.index_interval;
  	for(vector<Block>::iterator p=_blocks.begin(); p!=_blocks.end(); p++) {
		if(p->id%2!=ref_chain) continue;
   		_m=bfa[p->id].s; i2=((p->end-param.seed_size)/param.index_interval)*param.index_interval;
   		for(i=(p->begin/param.index_interval)*param.index_interval; i<=i2; i+=param.index_interval) {
   			__builtin_prefetch(index2+s_MakeSeed_1(_m+(i+prefetch)/SEGLEN,of-((i+prefetch)%SEGLEN)*2),1,0);
			index2[s_MakeSeed_1(_m+i/SEGLEN,of-(i%SEGLEN)*2)].n[ref_chain]++;
        }
  	}
}	

void RefSeq::AllocIndex() {
	KmerLoc *v; KmerLoc2 *u;
	bit32_t i, j, t, *ptr, block_size=1<<22, ptr_count=0; 
    if(param.RRBS_flag){
    	for(v=index,i=0; i<total_kmers; v++,i++) {
    		if(v->n1>0) {
    			v->loc1= new Hit[v->n1];
    			v->n1=0;
    		}
        }
    }
    else {
    	bit32_t *kmer_count=new bit32_t[total_kmers];
    	ptr = new bit32_t[block_size]; // 64MB
    	mem_pool.push_back(ptr);
        for(i=0,u=index2; i<total_kmers; i++,u++) {
        	t=u->n[1]+u->n[0]; kmer_count[i]=t;
            if(t==0) continue; 
            if(ptr_count+t>=block_size-PREFETCH_LOOP) {
            	if(t>=block_size-PREFETCH_LOOP) {
            		u->loc1= new bit32_t[t+PREFETCH_LOOP];
            		mem_pool.push_back(u->loc1);
            		for(j=0;j<PREFETCH_LOOP;j++) (u->loc1)[t+j]=0;
            		u->n[1]=0;
            		continue;
            	}
				for(j=0;j<PREFETCH_LOOP;j++) ptr[ptr_count+j]=0;
				ptr = new bit32_t[block_size];
				mem_pool.push_back(ptr);
				ptr_count=0;
			}
			u->loc1=ptr+ptr_count; ptr_count+=t;
            u->n[1]=0;
        }
        for(j=0;j<PREFETCH_LOOP;j++) ptr[ptr_count+j]=0;
        sort(kmer_count, kmer_count+total_kmers-1);
        param.max_kmer_num=kmer_count[(bit32_t)(total_kmers*(1-param.max_kmer_ratio))-1];
        //cout<<"kmer_threshold:"<<param.max_kmer_num<<" kmer_ratio:"<<param.max_kmer_ratio<<endl;
        delete [] kmer_count;
    }
}

void RefSeq::ReleaseIndex() {
    bit32_t i;
    KmerLoc *v; KmerLoc2 *u;
    if(param.RRBS_flag){
    	for(i=0,v=index; i<total_kmers; i++,v++) {
            if(v->n1) delete [] v->loc1;
    	}
        delete [] index;
    }
    else {
    	while(!mem_pool.empty()) {
    		delete [] mem_pool.back();
    		mem_pool.pop_back();
		}
        delete [] index2;
    }
}

void RefSeq::FillIndex() {
	bit32_t *_m, chr, chr1, tmp_offset, j,of=64-param.seed_size*2;
	KmerLoc *z; Hit tmphit;
    vector<ref_loc_t>::iterator it;
    if(param.RRBS_flag) {
        for(j=0;j<param.max_seedseg_num;j++) {
            for(chr=0;chr<_count;chr++) {
                tmphit.chr=chr|(j<<16); _m=bfa[chr].s;
                for(it=CCGG_index[j][chr].begin();it!=CCGG_index[j][chr].end();++it) {
                    z=index+s_MakeSeed_1(_m+(*it)/SEGLEN,of-((*it)%SEGLEN)*2);
                    tmphit.loc=(*it); 
                    z->loc1[z->n1++]=tmphit;
                }
                if(param.pairend||param.chains) {
                    chr1=chr^1;  tmp_offset=title[chr].rc_offset-param.seed_size;
                    tmphit.chr=chr|(j<<16)|0x1000000;
                    for(it=CCGG_index[j][chr1].begin();it!=CCGG_index[j][chr1].end();++it) {
                        z=index+s_MakeSeed_1(_m+(tmp_offset-*it)/SEGLEN,of-((tmp_offset-*it)%SEGLEN)*2);
                        tmphit.loc=(tmp_offset-*it); 
                        z->loc1[z->n1++]=tmphit;
                    }
                }
            }
        }
    }
    else {
		t_FillIndex(0);
		t_FillIndex(1);
    }

    //_a=0; _e=0; for(z=index,i=0; i<total_kmers; z++,i++) if(z->n1) {_a+=z->n1; _e++;} cout<<"index total:"<<_a<<" keys:"<<_e<<endl;
    //_a=0; _e=0; for(z=cindex, i=0; i<total_kmers; z++,i++) if(z->n1) {_a+=z->n1; _e++;} cout<<"cindex total:"<<_a<<" keys:"<<_e<<endl;
    /*
    cout<<"index\n";
    for(int i=0; i<total_kmers; i++){
        if(i==10623738){ 
            cout<<"*************************************\n";
            u=index2[i];
            for(int j=0; j<u[0]; j++) cout<<u[j]<<" "; cout<<endl;
        }
    }

    cout<<"cindex\n";
    for(i=0; i<total_kmers; i++){
        z=cindex+i;
        if(z->n1>0) {
            if (i==8957921) cout<<"*************************************\n";
            cout<<param.StrSeed(param.map3to4(i),param.seed_size)<<"\t"<<i<<"\t";
            for(int j=0; j<(cindex+i)->n1; j++) cout<<(int)z->id1[j]<<" "<<z->loc1[j]<<endl;
        }
    }

    for(ref_id_t chr=0;chr<_count;chr++){
        cout<<"chr"<<(int)chr<<endl;
        for(set<ref_loc_t>::iterator it=CCGG_index[chr].begin();it!=CCGG_index[chr].end();it++) cout<<" "<<*it; cout<<endl;
        for(set<ref_loc_t>::iterator it=CCGG_cindex[chr].begin();it!=CCGG_cindex[chr].end();it++) cout<<" "<<*it; cout<<endl;
    }
    */

}

void RefSeq::t_FillIndex(bit32_t ref_chain) {
	KmerLoc2 *z2; Hit tmphit;
   	bit32_t *_m, i2,of=64-param.seed_size*2,prefetch=PREFETCH_CRT_INDEX/4*param.index_interval, tmp=1-ref_chain;
    for(vector<Block>::iterator p=_blocks.begin(); p!=_blocks.end(); p++) {            
		if(p->id%2!=ref_chain) continue;
        tmphit.chr=p->id; _m=bfa[tmphit.chr].s;
     	i2=((p->end-param.seed_size)/param.index_interval)*param.index_interval;
        for(tmphit.loc=(p->begin/param.index_interval)*param.index_interval; tmphit.loc<=i2; tmphit.loc+=param.index_interval) {
            __builtin_prefetch(index2[s_MakeSeed_1(_m+(tmphit.loc+prefetch)/SEGLEN,of-((tmphit.loc+prefetch)%SEGLEN)*2)].loc1,1,0);
            z2=index2+s_MakeSeed_1(_m+tmphit.loc/SEGLEN, of-(tmphit.loc%SEGLEN)*2);
            z2->loc1[z2->n[tmp]++]=hit2int(tmphit);
    	}
	}
}

void RefSeq::CreateIndex() {
    bit32_t i;
	InitialIndex();
	CalKmerFreq();
	AllocIndex();
	FillIndex();
    if(param.RRBS_flag) 
        for(i=0;i<param.max_seedseg_num;i++) CCGG_index[i].clear();

/*
    for(i=0;i<total_kmers;i++) {
        cout<<"seed:"<<i<<" total:"<<index2[i][0]<<endl;
        for(j=1; j<=index2[i][0]; j++) cout<<index2[i][j]<<" "; cout<<endl;
        //for(k=0;k<4;k++) {
        while(true){
            cout<<"c"<<(index2[i][j]>>24)<<":"<<(index2[i][j]&0xffffff)<<" ";
            if((index2[i][j]&0xffffff)==index2[i][0]) break;
            j++;
        }
        cout<<endl;
    }
*/
}

pair<ref_loc_t,bit32_t> RefSeq::CCGG_seglen(ref_id_t chr, ref_loc_t pos, int readlen) {
    ref_id_t chr2=chr/2;
    bit32_t left, right, mid;
    ref_loc_t midval,seg_start, seg_end;
    CCGG_sites_chr=&CCGG_sites[chr2];
    left=0; //right=CCGG_sites[chr2].size()-1;
    right=CCGG_sites_chr->size()-1;
    while(left<right-1) {
        mid=(left+right)/2;
	//cout<<"pos:"<<pos<<"\tleft:"<<left<<" "<<CCGG_sites[chr2][left]<<"\tright:"<<right<<" "<<CCGG_sites[chr2][right]<<"\tmid:"<<mid<<" "<<CCGG_sites[chr2][mid]<<endl;
        if((midval=(*CCGG_sites_chr)[mid])==pos) {//return pair<ref_loc_t,int>(pos+1,CCGG_sites[chr2][mid+1]-midval+2);
            left=mid;right=mid+1;break;
        } 
        else if(midval<pos) left=mid;
        //if(CCGG_sites[chr2][mid]<=pos) left=mid;
        else right=mid;
    }
    //cout<<"endloop\n";
    //cout<<"pos:"<<pos<<"\tleft:"<<left<<" "<<CCGG_sites[chr2][left]<<"\tright:"<<right<<" "<<CCGG_sites[chr2][right]<<"\tmid:"<<mid<<" "<<CCGG_sites[chr2][mid]<<endl;
    //for (mid=left-2;mid<left+3;mid++) cout<<"\t"<<CCGG_sites[chr2][mid]; cout<<endl;
    seg_start=(*CCGG_sites_chr)[left];
    while(((seg_end=(*CCGG_sites_chr)[right]+param.digest_site.size()-param.digest_pos*2)<pos+readlen)&&(right<CCGG_sites_chr->size())) right++; 
        //cout<<"pos:"<<pos<<"\tleft:"<<left<<" "<<CCGG_sites[chr2][left]<<"\tright:"<<right<<" "<<CCGG_sites[chr2][right]<<"\tmid:"<<mid<<" "<<CCGG_sites[chr2][mid]<<endl;}
    //cout<<"ZP:"<<(seg_start+1)<<"  ZL:"<<(seg_end-seg_start)<<endl;
    //return pair<ref_loc_t,int>(CCGG_sites[chr2][left]+1, CCGG_sites[chr2][right]-CCGG_sites[chr2][left]+2);
    return pair<ref_loc_t,bit32_t>(seg_start+1, seg_end-seg_start);
}    

    
ref_loc_t RefSeq::hit2int(Hit h) {
    return ref_anchor[h.chr/2]+h.loc;
}

/*
Hit RefSeq::int2hit(ref_loc_t p, int c) {
    Hit tmphit; int i=0;
    while(p>=ref_anchor[i]) ++i;
    //cout<<"i="<<i<<" ref[i]="<<ref_anchor[i]<<endl;
    tmphit.chr=(i-1)*2+c;
    tmphit.loc=p-ref_anchor[i-1];
    return tmphit;
}
*/
    
gHit RefSeq::int2hit(gHit h, bit32_t c, int gap_size, bit32_t gap_pos) {
    bit32_t left=0, right=total_num, mid=total_num/2;
    gHit gh;
    while(left<right-1) {
        mid=(left+right)/2;
        if(h.loc>=ref_anchor[mid]) left=mid;
        else right=mid;
    }
    gh.chr=left*2+(c!=0);
    //cout<<"left="<<left<<" right="<<right<<endl;
    //cout<<"h.loc="<<h.loc<<" ref_anchor="<<ref_anchor[left]<<endl;
    gh.loc=h.loc-ref_anchor[left];
    gh.gap_size=gap_size;
    gh.gap_pos=gap_pos;
	if(c!=0) {
		//cout<<"h.loc="<<h.loc<<" readlen="<<c<<" "<<" rc_offset="<<title[h.chr].rc_offset<<endl;
	    gh.loc=title[gh.chr].rc_offset-c-gh.loc;
	   // cout<<"h.loc="<<h.loc<<endl;
        gh.gap_pos=(int)c+(gh.gap_size<0)*(gh.gap_size)-(int)gh.gap_pos;
   	    gh.loc-=gh.gap_size;
	}
	return gh;
}
